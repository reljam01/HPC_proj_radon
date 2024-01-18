#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <mpi.h>

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

void radonova_transformacija(Slika *img, int rank, int size, char *out_name)
{
    int centar_x = round(img->x/2);
    int centar_y = round(img->y/2);

    int ro_max = ceil(sqrt(img->x*img->x + img->y*img->y));

    int ro_poluprecnik = round(ro_max/2); 

    int najveci_teta = 360; 
    int pomeraj = 1;
    Slika *sinogram = (Slika *)malloc(sizeof(Slika));
    if (sinogram == NULL) {
        printf("Greska pri alociranju memorije!\n");
        exit(4);
    }
    
    sinogram->data = (Pixel *)calloc(2*ro_max*ceil(44/size),sizeof(Pixel));
    if(sinogram->data == NULL) {
        printf("Greska pri alociranju memorije!\n");
        exit(4);
    }
    sinogram->x = ro_max;
    sinogram->y = (int)ceil(44/size);
    for(int teta = (rank)*ceil(44/size)+1; teta < (rank+1)*ceil(44/size)+2; teta++) {

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
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* sinogram->x)].blue + img->data[(y+centar_x)*img->x + (int)x + centar_y].blue <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* sinogram->x)].blue += img->data[(y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* sinogram->x)].green + img->data[(y+centar_x)*img->x + (int)x + centar_y].green <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* sinogram->x)].green += img->data[(y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* sinogram->x)].red + img->data[(y+centar_x)*img->x + (int)x + centar_y].red <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* sinogram->x)].red += img->data[(y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }
    Slika *full_sinogram = NULL;
    full_sinogram = (Slika *)malloc(sizeof(Slika));
    if(full_sinogram == NULL) {
        printf("NULL SINOGRAM!\n");
        exit(4);
    }
    full_sinogram->data = (Pixel *)calloc(ro_max*najveci_teta,sizeof(Pixel));
    if(full_sinogram->data == NULL) {
        printf("NULL SINOGRAM!\n");
        exit(4);
    }
    full_sinogram->x = ro_max;
    full_sinogram->y = najveci_teta;

    //pocetak root dela (root radi ostatak linija)
    
    if (rank == 0)
    {
    for(int teta = 45-44%size; teta < 46; teta++) {

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
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* full_sinogram->x)].blue + img->data[(y+centar_x)*img->x + (int)x + centar_y].blue <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* full_sinogram->x)].blue += img->data[(y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* full_sinogram->x)].green + img->data[(y+centar_x)*img->x + (int)x + centar_y].green <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* full_sinogram->x)].green += img->data[(y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* full_sinogram->x)].red + img->data[(y+centar_x)*img->x + (int)x + centar_y].red <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (44 / size) - 1)* full_sinogram->x)].red += img->data[(y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }
    }
    
    //kraj main dela
    MPI_Gather(sinogram->data, 3*ro_max*(int)ceil(44/size), MPI_UNSIGNED_CHAR, full_sinogram->data, 3*ro_max*(int)ceil(44/size), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    

    free(sinogram->data);
    free(sinogram);

    sinogram = (Slika *)malloc(sizeof(Slika));
    if(sinogram == NULL) {
        printf("Greska pri alociranju!\n");
        exit(4);
    }
    sinogram->data = (Pixel *)calloc(2*ro_max*ceil(45/size),sizeof(Pixel));
    if(sinogram->data == NULL) {
        printf("Greska pri alociranju!\n");
        exit(4);
    }
    sinogram->x = ro_max;
    sinogram->y = (int)ceil(45/size);

    for(int teta = (rank)*ceil(45/size) + 45; teta < (rank+1)*ceil(45/size) + 45; teta++) {
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
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 45 - rank * (45 / size))* sinogram->x)].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255) 
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 45 - rank * (45 / size))* sinogram->x)].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 45 - rank * (45 / size))* sinogram->x)].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255) 
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 45 - rank * (45 / size))* sinogram->x)].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 45 - rank * (45 / size))* sinogram->x)].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255) 
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 45 - rank * (45 / size))* sinogram->x)].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }
    MPI_Gather(sinogram->data, 3*(ro_max)*ceil(45/size), MPI_UNSIGNED_CHAR, &(full_sinogram->data[ro_max*(45)]), 3*(ro_max)*ceil(45/size), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
    for(int teta = 90-(45%size); teta < 90; teta++) {
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
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255) 
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255) 
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255) 
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }
}

    free(sinogram->data);
    free(sinogram);

    sinogram = (Slika *)malloc(sizeof(Slika));
    if(sinogram == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->data = (Pixel *)calloc(2*ro_max*ceil(45/size),sizeof(Pixel));
    if(sinogram->data == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->x = ro_max;
    sinogram->y = (int)ceil(45/size);

    for(int teta = (rank)*ceil(45/size) + 90; teta < (rank+1)*ceil(45/size) + 90; teta++) {
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
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 90 - rank * (45 / size))* sinogram->x)].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].blue <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 90 - rank * (45 / size))* sinogram->x)].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 90 - rank * (45 / size))* sinogram->x)].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 90 - rank * (45 / size))* sinogram->x)].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 90 - rank * (45 / size))* sinogram->x)].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 90 - rank * (45 / size))* sinogram->x)].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }

    if (rank == 0) {
    for(int teta = 135-(45%size); teta < 135; teta++) {
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
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta- rank * (45 / size))* full_sinogram->x)].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].blue <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }
    }

    MPI_Gather(sinogram->data, 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, &(full_sinogram->data[ro_max*(90)]), 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    free(sinogram->data);
    free(sinogram);

    sinogram = (Slika *)malloc(sizeof(Slika));
    if (sinogram == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->data = (Pixel *)calloc(2*ro_max*ceil(45/size),sizeof(Pixel));
    if (sinogram->data == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->x = ro_max;
    sinogram->y = (int)ceil(45/size);

    for(int teta = (rank)*ceil(45/size) + 135; teta < (rank+1)*ceil(45/size) + 135; teta++) {
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
                if(sinogram->data[((ro_max - (int)ro - 1) + (teta - 135 - rank * (45 / size))* sinogram->x)].blue + img->data[(y+centar_x)*img->x + (int)x + centar_y].blue <= 255) {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 135 - rank * (45 / size))* sinogram->x)].blue += img->data[(y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if(sinogram->data[((ro_max - (int)ro - 1) + (teta - 135 - rank * (45 / size))* sinogram->x)].green + img->data[(y+centar_x)*img->x + (int)x + centar_y].green <= 255) {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 135 - rank * (45 / size))* sinogram->x)].green += img->data[(y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if(sinogram->data[((ro_max - (int)ro - 1) + (teta - 135 - rank * (45 / size))* sinogram->x)].red + img->data[(y+centar_x)*img->x + (int)x + centar_y].red <= 255) {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 135 - rank * (45 / size))* sinogram->x)].red += img->data[(y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }

    if(rank == 0) {
    for(int teta = 180-45%size; teta < 180; teta++) {
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
                if(full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue + img->data[(y+centar_x)*img->x + (int)x + centar_y].blue <= 255) {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue += img->data[(y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if(full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green + img->data[(y+centar_x)*img->x + (int)x + centar_y].green <= 255) {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green += img->data[(y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if(full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red + img->data[(y+centar_x)*img->x + (int)x + centar_y].red <= 255) {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red += img->data[(y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }
    }

    MPI_Gather(sinogram->data, 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, &(full_sinogram->data[ro_max*(135)]), 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    free(sinogram->data);
    free(sinogram);

    sinogram = (Slika *)malloc(sizeof(Slika));
    if(sinogram == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->data = (Pixel *)calloc(2*ro_max*ceil(45/size),sizeof(Pixel));
    if(sinogram->data == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->x = ro_max;
    sinogram->y = (int)ceil(45/size);

    for(int teta = (rank)*ceil(45/size) + 180; teta <= (rank+1)*ceil(45/size) + 180; teta++) {

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
                
                if(sinogram->data[((ro_max - (int)ro - 1) + (teta - 180 - rank * (45 / size))* sinogram->x)].blue + img->data[(y+centar_x)*img->x + (int)x + centar_y].blue <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 180 - rank * (45 / size))* sinogram->x)].blue += img->data[(y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if(sinogram->data[((ro_max - (int)ro - 1) + (teta - 180 - rank * (45 / size))* sinogram->x)].green + img->data[(y+centar_x)*img->x + (int)x + centar_y].green <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 180 - rank * (45 / size))* sinogram->x)].green += img->data[(y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if(sinogram->data[((ro_max - (int)ro - 1) + (teta - 180 - rank * (45 / size))* sinogram->x)].red + img->data[(y+centar_x)*img->x + (int)x + centar_y].red <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 180 - rank * (45 / size))* sinogram->x)].red += img->data[(y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }

    if(rank == 0) {
    for(int teta = 225-(45%size); teta < 225; teta++) {

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
                
                if(full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue + img->data[(y+centar_x)*img->x + (int)x + centar_y].blue <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue += img->data[(y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if(full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green + img->data[(y+centar_x)*img->x + (int)x + centar_y].green <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green += img->data[(y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if(full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red + img->data[(y+centar_x)*img->x + (int)x + centar_y].red <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red += img->data[(y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }
    }

    MPI_Gather(&sinogram->data[ro_max], 3*(ro_max)*ceil(45/size), MPI_UNSIGNED_CHAR, &(full_sinogram->data[ro_max*(180)]), 3*(ro_max)*ceil(45/size), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    free(sinogram->data);
    free(sinogram);

    sinogram = (Slika *)malloc(sizeof(Slika));
    if (sinogram == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->data = (Pixel *)calloc(2*ro_max*ceil(45/size),sizeof(Pixel));
    if (sinogram->data == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->x = ro_max;
    sinogram->y = (int)ceil(45/size);

    for(int teta = (rank)*ceil(45/size) + 225; teta < (rank+1)*ceil(45/size) + 225; teta++) {
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
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 225 - rank * (45 / size))* sinogram->x)].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255) {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 225 - rank * (45 / size))* sinogram->x)].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 225 - rank * (45 / size))* sinogram->x)].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255) {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 225 - rank * (45 / size))* sinogram->x)].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 225 - rank * (45 / size))* sinogram->x)].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255) {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 225 - rank * (45 / size))* sinogram->x)].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }

    if (rank == 0) {
    for(int teta = 270-(45%size); teta < 270; teta++) {
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
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue <= 255) {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].blue/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].green <= 255) {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y ].red <= 255) {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }
    }

    MPI_Gather(sinogram->data, 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, &(full_sinogram->data[ro_max*(225)]), 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    free(sinogram->data);
    free(sinogram);

    sinogram = (Slika *)malloc(sizeof(Slika));
    if(sinogram == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->data = (Pixel *)calloc(2*ro_max*ceil(45/size),sizeof(Pixel));
    if(sinogram->data == NULL) {
        printf("Greska pri alokaciji!\n");
        exit(4);
    }
    sinogram->x = ro_max;
    sinogram->y = (int)ceil(45/size);

    for(int teta = (rank)*(45/size) + 270; teta < (rank+1)*(45/size) + 270; teta++) {
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
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 270 - rank * (45 / size))* sinogram->x)].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].blue <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 270 - rank * (45 / size))* sinogram->x)].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 270 - rank * (45 / size))* sinogram->x)].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 270 - rank * (45 / size))* sinogram->x)].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 270 - rank * (45 / size))* sinogram->x)].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 270 - rank * (45 / size))* sinogram->x)].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }

    if(rank == 0) {
    for(int teta = 315 - (45%size); teta < 315; teta++) {
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
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].blue <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].blue += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].green += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red + img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red <= 255)
                {
                full_sinogram->data[((ro_max - (int)ro - 1) + (teta - rank * (45 / size))* full_sinogram->x)].red += img->data[((int)y+centar_x)*img->x + (int)x + centar_y].red/12;
                }
            }
        }
    }
    }

    MPI_Gather(sinogram->data, 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, &(full_sinogram->data[ro_max*(270)]), 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    free(sinogram->data);
    free(sinogram);

    sinogram = (Slika *)malloc(sizeof(Slika));
    if(sinogram == NULL) {
        printf("Greska pri alociranju!\n");
        exit(4);
    }
    sinogram->data = (Pixel *)calloc(2*ro_max*ceil(45/size),sizeof(Pixel));
    if(sinogram->data == NULL) {
        printf("Greska pri alociranju!\n");
        exit(4);
    }
    sinogram->x = ro_max;
    sinogram->y = (int)ceil(45/size);

    for(int teta = (rank)*ceil(45/size) + 315; teta < (rank+1)*ceil(45/size) + 315; teta++) {
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
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 315 - rank * (45 / size))* sinogram->x)].blue + img->data[(y+centar_x)*img->x + (int)x + centar_y].blue <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 315 - rank * (45 / size))* sinogram->x)].blue += img->data[(y+centar_x)*img->x + (int)x + centar_y].blue/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 315 - rank * (45 / size))* sinogram->x)].green + img->data[(y+centar_x)*img->x + (int)x + centar_y].green <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 315 - rank * (45 / size))* sinogram->x)].green += img->data[(y+centar_x)*img->x + (int)x + centar_y].green/12;
                }
                if (sinogram->data[((ro_max - (int)ro - 1) + (teta - 315 - rank * (45 / size))* sinogram->x)].red + img->data[(y+centar_x)*img->x + (int)x + centar_y].red <= 255)
                {
                sinogram->data[((ro_max - (int)ro - 1) + (teta - 315 - rank * (45 / size))* sinogram->x)].red += img->data[(y+centar_x)*img->x + (int)x + centar_y].red/12;
                }

            }
        }
    }

    MPI_Gather(sinogram->data, 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, &(full_sinogram->data[ro_max*(315)]), 3*ro_max*ceil(45/size), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        napravi_sliku(out_name,full_sinogram);
    }

    free(sinogram->data);
    free(sinogram);

    free(full_sinogram->data);
    free(full_sinogram);
}

int main(int argc, char **argv){
    MPI_Init(&argc, &argv);

    int rank, size;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double uk_start = omp_get_wtime();

    Slika *image;
    image = ucitaj_sliku("kvadrati.ppm");
    double start = omp_get_wtime();
    radonova_transformacija(image, rank, size, "kvadrati_sin.ppm");
    double end = omp_get_wtime();
    if (rank == 0)
    {
        printf("Velicina ulazne slike kvadrati.ppm: %dx%d\n",image->x,image->y);
        printf("Vreme potrebno za Radonovu transformaciju slike kvadrati.ppm: %.5lf\n",end-start);
    }

    image = ucitaj_sliku("shepp_logan_128.ppm");
    start = omp_get_wtime();
    radonova_transformacija(image, rank, size, "shepp_logan_128_sin.ppm");
    end = omp_get_wtime();
    if (rank == 0)
    {
        printf("Velicina ulazne slike shepp_logan_128.ppm: %dx%d\n",image->x,image->y);
        printf("Vreme potrebno za Radonovu transformaciju slike shepp_logan_128.ppm: %.5lf\n",end-start);
    }

    image = ucitaj_sliku("shepp_logan_512.ppm");
    start = omp_get_wtime();
    radonova_transformacija(image, rank, size, "shepp_logan_512_sin.ppm");
    end = omp_get_wtime();
    if (rank == 0)
    {
        printf("Velicina ulazne slike shepp_logan_512.ppm: %dx%d\n",image->x,image->y);
        printf("Vreme potrebno za Radonovu transformaciju slike shepp_logan_512.ppm: %.5lf\n",end-start);
    }

    image = ucitaj_sliku("eye.ppm");
    start = omp_get_wtime();
    radonova_transformacija(image, rank, size, "eye_sin.ppm");
    end = omp_get_wtime();
    if (rank == 0)
    {
        printf("Velicina ulazne slike eye.ppm: %dx%d\n",image->x,image->y);
        printf("Vreme potrebno za Radonovu transformaciju slike eye.ppm: %.5lf\n",end-start);
    }

    image = ucitaj_sliku("ct_brain.ppm");
    start = omp_get_wtime();
    radonova_transformacija(image, rank, size, "ct_brain_sin.ppm");
    end = omp_get_wtime();
    if (rank == 0)
    {
        printf("Velicina ulazne slike ct_brain.ppm: %dx%d\n",image->x,image->y);
        printf("Vreme potrebno za Radonovu transformaciju slike ct_brain.ppm: %.5lf\n",end-start);
    }

    double uk_end = omp_get_wtime();
    if (rank == 0) {
        printf("\nUkupno vreme obrade slika: %.5lf\n",uk_end - uk_start);
    }

    MPI_Finalize();
}
