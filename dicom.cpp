#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcitem.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcdatset.h"
#include "dcmtk/dcmdata/dcdicdir.h"
#include "dcmtk/dcmdata/dcdirrec.h"
#include "dcmtk/dcmdata/dcbytstr.h"
#include <dcmtk/dcmdata/dcstack.h>
#include <dcmtk/ofstd/oftypes.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <memory>
#include <cstdio>
#include <cmath>

typedef struct {
    Uint16 I,Ct,Cp;
} DICtCp;

DICtCp LUT[0xFFFF];

class Image {
public:
    Image(int r, int c) : data{new Uint16[r*c]}, rows{r}, columns{c} {}
    ~Image() {}
    Uint16& operator[](std::size_t i) {
        return data[i];
    }
    int get_rows() {
        return rows;
    }
    int get_columns() {
        return columns;
    }
private:
    std::shared_ptr<Uint16[]> data;
    int rows,columns;
};


double ttolen(double t) {
    return 0.5*t*sqrt(1.0441+70.56*t*t)+0.0621488*asinh(8.22069*t);
}

double lentot(double len) {
    double max = 0.750;
    double min = 0;
    double iter = 0.375;
    double val;
    while(fabs((val = ttolen(iter)) - len) > 0.00000001) {
        if(val < len) {
            min = iter;
        } else {
            max = iter;
        }
        iter = (max + min)/2;
    }
    return iter;
}

void updateLUT(int max) {
    for(int i = 0; i < 0xFFFF; i++) {
        if(i <= max) {
            double val = lentot(((double)i/(double)max)*2.51809);
            LUT[i].I = (val*219 + 16)*4;
            LUT[i].Ct = ((sin(val*40)*val*0.21)*224*2 + 128)*4;
            LUT[i].Cp = ((cos(val*40)*val*0.21)*224 + 128)*4;
        } else {
            LUT[i].I = 590;
            LUT[i].Ct = 120;
            LUT[i].Cp = 406;
        }
    }
}

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cout << "Must enter location of DICOMDIR.\n";
        return 0;
    }
    updateLUT(4000);
    Uint16 *Is = new Uint16[3840*2160];
    Uint16 *Cts = new Uint16[1920*1080];
    Uint16 *Cps = new Uint16[1920*1080];
    OFString location = argv[1];
    OFString dirlocation = location + "/DICOMDIR";
    DcmDicomDir dicomdir{dirlocation.c_str()};
    DcmDirectoryRecord record = dicomdir.getRootRecord();
    DcmDirectoryRecord *series = record.getSub(0)->getSub(0);
    for(int i = 0; i < series->cardSub(); i++) {
        DcmDirectoryRecord *study = series->getSub(i);
        std::vector<Image> imgs;
        Sint16 max = 0;
        for(int j = 0; j < study->cardSub(); j++) {
            DcmDirectoryRecord *image = study->getSub(j);
            OFString prefix,suffix;
            image->findAndGetOFString(DCM_ReferencedFileID,prefix,0);
            image->findAndGetOFString(DCM_ReferencedFileID,suffix,1);
            OFString imglocation = location + "/" + prefix + "/" + suffix;
            DcmFileFormat file;
            file.loadFile(imglocation);
            DcmDataset *data = file.getDataset();
            Uint16 *values;
            Uint16 rows,columns;
            Sint16 tempmax;
            unsigned long count;
            data->findAndGetSint16(DCM_LargestImagePixelValue,tempmax);
            data->findAndGetUint16Array(DCM_PixelData,(const Uint16*&)values,&count);
            data->findAndGetUint16(DCM_Rows,rows);
            data->findAndGetUint16(DCM_Columns,columns);
            max = std::max(max,tempmax);
            if(count > 0) {
                Image temp{rows,columns};
                Uint16 tempmax2 = 0;
                for(int k = 0; k < rows*columns; k++) {
                    temp[k] = values[k];
                    tempmax2 = std::max(tempmax2,values[k]);
                }
                imgs.push_back(temp);
            }
        }
        if(max != 0) {
            updateLUT(max);
        }
        for(auto& img : imgs) {
            for(int i = 0; i < 3840*2160; i++) {
                Is[i] = 64;
            }
            for(int i = 0; i < 1920*1080; i++) {
                Cts[i] = 512;
                Cps[i] = 512;
            }
            int height = std::min(img.get_rows()*4,2160);
            int width = std::min(img.get_columns()*4,3840);
            for(int y = 0; y < height; y += 2) {
                for(int x = 0; x < width; x += 2) {
                    DICtCp val = LUT[img[(y/4)*img.get_columns() + (x/4)]];
                    Is[y*3840 + x] = val.I;
                    Is[(y+1)*3840 + x] = val.I;
                    Is[y*3840 + x+1] = val.I;
                    Is[(y+1)*3840 + x + 1] = val.I;
                    Cts[(y/2)*1920 + (x/2)] = val.Ct;
                    Cps[(y/2)*1920 + (x/2)] = val.Cp;
                }
            }
            std::fwrite(Is,2,3840*2160,stdout);
            std::fwrite(Cts,2,1920*1080,stdout);
            std::fwrite(Cps,2,1920*1080,stdout);
            /*
            std::fwrite(Is,2,3840*2160,stdout);
            std::fwrite(Cts,2,1920*1080,stdout);
            std::fwrite(Cps,2,1920*1080,stdout);
            std::fwrite(Is,2,3840*2160,stdout);
            std::fwrite(Cts,2,1920*1080,stdout);
            std::fwrite(Cps,2,1920*1080,stdout);*/
        }
    }
    delete[] Is;
    delete[] Cts;
    delete[] Cps;
}