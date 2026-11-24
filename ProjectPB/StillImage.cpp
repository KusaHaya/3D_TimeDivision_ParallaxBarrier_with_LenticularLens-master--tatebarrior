#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <stdio.h>
#include <stdlib.h>
#include "AppState.h"
#include "StillImage.h"
#include <GL/glut.h>

char filePath[32]; // store filename

// 静止画表示用の左右テクスチャ。
unsigned char image_texture[2][SIM_H][SIM_W][3];

int ppm_reader(char *filename, unsigned char *pimage)
{
	char buff[16];
	FILE *fp;
	int c, rgb_comp_color;
	//open PPM file for reading
	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		exit(1);
	}

	//read image format
	if (!fgets(buff, sizeof(buff), fp)) {
		perror(filename);
		exit(1);
	}

	//check the image format
	if (buff[0] != 'P' || buff[1] != '6') {
		fprintf(stderr, "Invalid image format (must be 'P6')\n");
		exit(1);
	}

	//alloc memory form image
	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//check for comments
	c = getc(fp);
	while (c == '#') {
		while (getc(fp) != '\n');
		c = getc(fp);
	}
	int width, height;
	ungetc(c, fp);
	//read image size information
	if (fscanf(fp, "%d %d", &width, &height) != 2) {
		fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
		exit(1);
	}

	//read rgb component
	if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
		fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
		exit(1);
	}

	while (fgetc(fp) != '\n');
	//memory allocation for pixel data

	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//read pixel data from file
	if (fread(pimage, 3 * width, height, fp) != height) {
		fprintf(stderr, "Error loading image '%s'\n", filename);
		exit(1);
	}
	fclose(fp);
	return 0; // 戻り値は未使用。MSVCのC4716（戻り値なし）を避ける。
}

void LoadStillImageTextures()
{
		sprintf(filePath, "./images/IDW/%03dl.ppm", img);
		ppm_reader(filePath, &image_texture[0][0][0][0]);
		sprintf(filePath, "./images/IDW/%03dr.ppm", img);// 右目画像のファイル名を設定する。
		ppm_reader(filePath, &image_texture[1][0][0][0]);

		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, imageL);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIM_W, SIM_H, 0, GL_RGB, GL_UNSIGNED_BYTE, image_texture[0]);

		glBindTexture(GL_TEXTURE_2D, imageR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIM_W, SIM_H, 0, GL_RGB, GL_UNSIGNED_BYTE, image_texture[1]);
}
