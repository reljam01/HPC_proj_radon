#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

typedef struct {
     unsigned char red,green,blue;
} Pixel;

typedef struct {
     int x, y;
     Pixel *data;
} Slika;

#define RGB_COMPONENT_COLOR 255

Slika *ucitaj_sliku(const char *filename)
{
         char buff[16];
         Slika *img;
         FILE *fp;
         int c, rgb_comp_color;
         //open PPM file for reading
         fp = fopen(filename, "rb");
         if (!fp) {
              fprintf(stderr, "Datoteka nije uspesno otvorena: '%s'\n", filename);
              exit(1);
         }

         //read image format
         if (!fgets(buff, sizeof(buff), fp)) {
              perror(filename);
              exit(2);
         }

    //check the image format
    if (buff[0] != 'P' || buff[1] != '6') {
         fprintf(stderr, "Nije validan format slike, mora biti PPM\n");
         exit(3);
    }

    //alloc memory form image
    img = (Slika *)malloc(sizeof(Slika));
    if (!img) {
         fprintf(stderr, "Greska pri alociranju memorije za sliku\n");
         exit(4);
    }

    //preskok komentara slike
    c = getc(fp);
    while (c == '#') {
    while (getc(fp) != '\n') ;
         c = getc(fp);
    }

    ungetc(c, fp);
    //velicina slike
    if (fscanf(fp, "%d %d", &img->x, &img->y) != 2) {
         fprintf(stderr, "Pogresna velicina slike!\n");
         exit(1);
    }

    //rgb komponenta
    if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
         fprintf(stderr, "Ne postoje definisane nijanse!\n");
         exit(1);
    }

    //provera rgb komponente
    if (rgb_comp_color!= RGB_COMPONENT_COLOR) {
         fprintf(stderr, "'%s' ima pogresne nijanse!\n", filename);
         exit(1);
    }

    while (fgetc(fp) != '\n') ;
    //alokacija niza (matrice) za piksele slike
    img->data = (Pixel*)malloc(img->x * img->y * sizeof(Pixel));

    if (!img) {
         fprintf(stderr, "Neuspesno alociran niz piksela slike!\n");
         exit(4);
    }

    //Citanje piksela iz fajla
    if (fread(img->data, 3 * img->x, img->y, fp) != img->y) {
         fprintf(stderr, "Greska pri ucitavanju piksela iz: '%s'\n", filename);
         exit(1);
    }

    fclose(fp);
    return img;
}
void napravi_sliku(const char *filename, Slika *img)
{
    FILE *fp;
    //open file for output
    fp = fopen(filename, "wb");
    if (!fp) {
         fprintf(stderr, "Unable to open file '%s'\n", filename);
         exit(1);
    }

    //write the header file
    //image format
    fprintf(fp, "P6\n");

    //image size
    fprintf(fp, "%d %d\n",img->x,img->y);

    // rgb component depth
    fprintf(fp, "%d\n",RGB_COMPONENT_COLOR);

    // pixel data
    fwrite(img->data, 3 * img->x, img->y, fp);
    fclose(fp);
}

Slika *radonova_transformacija(Slika *img)
{
    int centar_x = round(img->x/2);
    int centar_y = round(img->y/2);

    int ro_max = ceil(sqrt(img->x*img->x + img->y*img->y));

    int ro_poluprecnik = round(ro_max/2); 

    int najveci_teta = 360; 
    int pomeraj = 1;

    Slika *sinogram = (Slika *)malloc(sizeof(Slika));
    if (sinogram == NULL) {
        printf("Greska pri zauzimanju memorije!\n");
        exit(4);
    }
    sinogram->data = (Pixel *)calloc(ro_max*najveci_teta,sizeof(Pixel));
    if (sinogram->data == NULL) {
        printf("Greska pri zauzimanju memorije!\n");
        exit(4);
    }
    sinogram->x = najveci_teta;
    sinogram->y = ro_max;

    
    for(int teta = 1; teta < 45; teta++) {

        
        double cos_teta = cos(teta*3.14159/180);
        double sin_teta = sin(teta*3.14159/180);
        
        double a = -cos_teta/sin_teta;


        for (int ro = 1; ro < ro_max; ro++) {
            
            double ro_d = ro - ro_poluprecnik;
            
            double b = ro_d/sin_teta;
            
            int ymax,ymin;
            if (round(-a*centar_y+b) > centar_x-1) {
                ymax = centar_x-1;
            } else {
                ymax = round(-a*centar_y+b);
            }
            if (round(a*centar_y+b) > -centar_x) {
                ymin = round(a*centar_y+b);
            } else {
                ymin = -centar_x;
            }

            
            for(int y = ymin; y < ymax; y++) {
                
                double x = (y-b)/a;

                
                if (x < -centar_y) {
                    x = -centar_y;
                }
                if (x > centar_y-1) {
                    x = centar_y-1;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red/12;
                }
            }
        }
    }

    for(int teta = 45; teta < 90; teta++) {
        double cos_teta = cos(teta*3.1415/180);
        double sin_teta = sin(teta*3.1415/180);
        double a = -cos_teta/sin_teta; 

        for (int ro = 1; ro < ro_max; ro++) {
            int ro_d = ro - ro_poluprecnik; 
            double b = ro_d/sin_teta;
            int xmax,xmin;
            if (round(-centar_x-b)/a > centar_y-1) {
                xmax = centar_y-1;
            } else {
                xmax = round(-centar_x-b)/a;
            }
            if (round(centar_x-b)/a > -centar_y) {
                xmin = round(centar_x-b)/a;
            } else {
                xmin = -centar_y;
            }
            for(int x = xmin; x < xmax; x++) {
                double y = a*x+b;
                if (y < -centar_x) {
                    y = -centar_x;
                }
                if (y > centar_x-1) {
                    y = centar_x-1;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red/12;
                }
            }
        }
    }

    for(int teta = 90; teta < 135; teta++) {
        double cos_teta = cos(teta*3.1415/180);
        double sin_teta = sin(teta*3.1415/180);
        double a = -cos_teta/sin_teta; 

        for (int ro = 1; ro < ro_max; ro++) {
            int ro_d = ro - ro_poluprecnik; 
            double b = ro_d/sin_teta; 
            int xmax,xmin;
            if (round(centar_x-b)/a > centar_y-1) {
                xmax = centar_y-1;
            } else {
                xmax = round(centar_x-b)/a;
            }
            if (round(-centar_x-b)/a > -centar_y) {
                xmin = round(-centar_x-b)/a;
            } else {
                xmin = -centar_y;
            }
            for(int x = xmin; x < xmax; x++) {
                double y = a*x+b;
                if (y < -centar_x) {
                    y = -centar_x;
                }
                if (y > centar_x-1) {
                    y = centar_x-1;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red/12;
                }
            }
        }
    }

    for(int teta = 135; teta <= 180; teta++) {
        double cos_teta = cos(teta*3.1415/180);
        double sin_teta = sin(teta*3.1415/180);
        double a = -cos_teta/sin_teta; 

        for (int ro = 1; ro < ro_max; ro++) {
            int ro_d = ro - ro_poluprecnik; 
            double b = ro_d/sin_teta; 
            int ymax,ymin;
            if (round(a*centar_y+b) > centar_x-1) {
                ymax = centar_x-1;
            } else {
                ymax = round(a*centar_y+b);
            }
            if (round(-a*centar_y+b) > - centar_x) {
                ymin = round(-a*centar_y+b);
            } else {
                ymin = -centar_x;
            }
            for(int y = ymin; y < ymax; y++) {
                double x = (y-b)/a;
                if (x < -centar_y) {
                    x = -centar_y;
                }
                if (x > centar_y-1) {
                    x = centar_y-1;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red/12;
                }
            }
        }
    }

    for(int teta = 181; teta < 225; teta++) {

        double cos_teta = cos(teta*3.14159/180);
        double sin_teta = sin(teta*3.14159/180);
        double a = -cos_teta/sin_teta;

        for (int ro = 1; ro < ro_max; ro++) {
            int ro_d = ro - ro_poluprecnik;
            double b = ro_d/sin_teta;
            int ymax,ymin;
            if (round(-a*centar_y+b) > centar_x-1) {
                ymax = centar_x-1;
            } else {
                ymax = round(-a*centar_y+b);
            }
            if (round(a*centar_y+b) > -centar_x) {
                ymin = round(a*centar_y+b);
            } else {
                ymin = -centar_x;
            }

            for(int y = ymin; y < ymax; y++) {
                double x = (y-b)/a;

                if (x < -centar_y) {
                    x = -centar_y;
                }
                if (x > centar_y-1) {
                    x = centar_y-1;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red/12;
                }
            }
        }
    }

    for(int teta = 225; teta < 270; teta++) {
        double cos_teta = cos(teta*3.1415/180);
        double sin_teta = sin(teta*3.1415/180);
        double a = -cos_teta/sin_teta;

        for (int ro = 1; ro < ro_max; ro++) {
            int ro_d = ro - ro_poluprecnik; 
            double b = ro_d/sin_teta;
            int xmax,xmin;
            if (round(-centar_x-b)/a > centar_y-1) {
                xmax = centar_y-1;
            } else {
                xmax = round(-centar_x-b)/a;
            }
            if (round(centar_x-b)/a > -centar_y) {
                xmin = round(centar_x-b)/a;
            } else {
                xmin = -centar_y;
            }
            for(int x = xmin; x < xmax; x++) {
                double y = a*x+b;
                if (y < -centar_x) {
                    y = -centar_x;
                }
                if (y > centar_x-1) {
                    y = centar_x-1;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red/12;
                }
            }
        }
    }

    for(int teta = 270; teta < 315; teta++) {
        double cos_teta = cos(teta*3.1415/180);
        double sin_teta = sin(teta*3.1415/180);
        double a = -cos_teta/sin_teta; 

        for (int ro = 1; ro < ro_max; ro++) {
            int ro_d = ro - ro_poluprecnik; 
            double b = ro_d/sin_teta; 
            int xmax,xmin;
            if (round(centar_x-b)/a > centar_y-1) {
                xmax = centar_y-1;
            } else {
                xmax = round(centar_x-b)/a;
            }
            if (round(-centar_x-b)/a > -centar_y) {
                xmin = round(-centar_x-b)/a;
            } else {
                xmin = -centar_y;
            }
            for(int x = xmin; x < xmax; x++) {
                double y = a*x+b;
                if (y < -centar_x) {
                    y = -centar_x;
                }
                if (y > centar_x-1) {
                    y = centar_x-1;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red/12;
                }
            }
        }
    }

    for(int teta = 315; teta < 360; teta++) {
        double cos_teta = cos(teta*3.1415/180);
        double sin_teta = sin(teta*3.1415/180);
        double a = -cos_teta/sin_teta;

        for (int ro = 1; ro < ro_max; ro++) {
            int ro_d = ro - ro_poluprecnik;
            double b = ro_d/sin_teta;
            int ymax,ymin;
            if (round(a*centar_y+b) > centar_x-1) {
                ymax = centar_x-1;
            } else {
                ymax = round(a*centar_y+b);
            }
            if (round(-a*centar_y+b) > - centar_x) {
                ymin = round(-a*centar_y+b);
            } else {
                ymin = -centar_x;
            }
            for(int y = ymin; y < ymax; y++) {
                double x = (y-b)/a;
                if (x < -centar_y) {
                    x = -centar_y;
                }
                if (x > centar_y-1) {
                    x = centar_y-1;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green/12;
                }
                if(sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255)
                {
                sinogram->data[(ro_max - ro - 1) * najveci_teta + najveci_teta - teta - 1].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red/12;
                }
            }
        }
    }

    return sinogram;
}

int main(){

    double uk_start = omp_get_wtime();

    Slika *image;
    image = ucitaj_sliku("kvadrati.ppm");
    printf("Velicina ulazne slike kvadrati.ppm: %dx%d\n",image->x,image->y);
    double start = omp_get_wtime();
    Slika *sinogram = radonova_transformacija(image);
    double end = omp_get_wtime();
    printf("Vreme potrebno za Radonovu transformaciju slike kvadrati.ppm: %.5lf\n",end-start);
    napravi_sliku("kvadrati_sin.ppm",sinogram);
    free(image->data);
    free(sinogram->data);
    free(image);
    free(sinogram);

    image = ucitaj_sliku("shepp_logan_128.ppm");
    printf("\nVelicina ulazne slike shepp_logan_128.ppm: %dx%d\n",image->x,image->y);
    start = omp_get_wtime();
    sinogram = radonova_transformacija(image);
    end = omp_get_wtime();
    printf("Vreme potrebno za Radonovu transformaciju slike shepp_logan_128.ppm: %.5lf\n",end-start);
    napravi_sliku("shepp_logan_128_sin.ppm",sinogram);
    free(image->data);
    free(sinogram->data);
    free(image);
    free(sinogram);

    image = ucitaj_sliku("shepp_logan_512.ppm");
    printf("\nVelicina ulazne slike shepp_logan_512.ppm: %dx%d\n",image->x,image->y);
    start = omp_get_wtime();
    sinogram = radonova_transformacija(image);
    end = omp_get_wtime();
    printf("Vreme potrebno za Radonovu transformaciju slike shepp_logan_512.ppm: %.5lf\n",end-start);
    napravi_sliku("shepp_logan_512_sin.ppm",sinogram);
    free(image->data);
    free(sinogram->data);
    free(image);
    free(sinogram);

    image = ucitaj_sliku("eye.ppm");
    printf("\nVelicina ulazne slike eye.ppm: %dx%d\n",image->x,image->y);
    start = omp_get_wtime();
    sinogram = radonova_transformacija(image);
    end = omp_get_wtime();
    printf("Vreme potrebno za Radonovu transformaciju slike eye.ppm: %.5lf\n",end-start);
    napravi_sliku("eye_sin.ppm",sinogram);
    free(image->data);
    free(sinogram->data);
    free(image);
    free(sinogram);

    image = ucitaj_sliku("ct_brain.ppm");
    printf("\nVelicina ulazne slike ct_brain.ppm: %dx%d\n",image->x,image->y);
    start = omp_get_wtime();
    sinogram = radonova_transformacija(image);
    end = omp_get_wtime();
    printf("Vreme potrebno za Radonovu transformaciju slike ct_brain.ppm: %.5lf\n",end-start);
    napravi_sliku("ct_brain_sin.ppm",sinogram);
    free(image->data);
    free(sinogram->data);
    free(image);
    free(sinogram);

    double uk_end = omp_get_wtime();
    printf("\nUkupno vreme obrade slika: %.5lf\n",uk_end - uk_start);

    return 0;
}
