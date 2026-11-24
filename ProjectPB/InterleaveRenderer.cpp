#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <stdio.h>
#include "AppState.h"
#include "ShaderLoader.h"
#include "InterleaveRenderer.h"

GLuint shaderProgram;
GLuint videoshaderProgram;

// --- uniform location cache (image shader) ---
GLint u_img_leftTexture = -1;
GLint u_img_rightTexture = -1;
GLint u_img_leftEnabled = -1;
GLint u_img_rightEnabled = -1;
GLint u_img_haba = -1;
GLint u_img_totalShift = -1;
GLint u_img_timeStep = -1;
GLint u_img_manualShift = -1;
GLint u_img_showAlignmentCenter = -1;
GLint u_img_alignmentCenterSub = -1;
GLint u_img_alignmentViewportHeight = -1;
GLint u_img_correctionThreshold = -1;

GLint u_img_slantY = -1;
GLint u_vid_slantY = -1;
// 現在は使用しない色補正uniform。
//GLint u_img_rgbGain = -1;
//GLint u_img_gamma = -1;
//GLint u_img_crosstalk = -1;
//GLint u_img_enableColorCorrection = -1;

// --- uniform location cache (video shader) ---
GLint u_vid_sbsTexture = -1;
GLint u_vid_leftEnabled = -1;
GLint u_vid_rightEnabled = -1;
GLint u_vid_haba = -1;
GLint u_vid_totalShift = -1;
GLint u_vid_timeStep = -1;
GLint u_vid_manualShift = -1;
GLint u_vid_showAlignmentCenter = -1;
GLint u_vid_alignmentCenterSub = -1;
GLint u_vid_alignmentViewportHeight = -1;
GLint u_vid_correctionThreshold = -1;
// 現在は使用しない色補正uniform。
//GLint u_vid_rgbGain = -1;
//GLint u_vid_gamma = -1;
//GLint u_vid_crosstalk = -1;
//GLint u_vid_enableColorCorrection = -1;


// 画面全体に描画する四角形を、2枚の三角形で定義する。
GLuint quadVBO, quadVAO;
GLfloat quadVertices[] = {
	// 頂点位置(x, y, z)とテクスチャ座標(u, v)。
	-1.0f,  1.0f, 0.0f,  0.0f, 0.0f, // 画像の上下に合わせたUV座標。
	-1.0f, -1.0f, 0.0f,  0.0f, 1.0f, // 画像の上下に合わせたUV座標。
	 1.0f, -1.0f, 0.0f,  1.0f, 1.0f, // 画像の上下に合わせたUV座標。

	-1.0f,  1.0f, 0.0f,  0.0f, 0.0f, // 画像の上下に合わせたUV座標。
	 1.0f, -1.0f, 0.0f,  1.0f, 1.0f, // 画像の上下に合わせたUV座標。
	 1.0f,  1.0f, 0.0f,  1.0f, 0.0f  // 画像の上下に合わせたUV座標。
};

void setupQuad() {
	// VAOを作成してバインドする。
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);

	// VBOを作成してバインドする。
	glGenBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	// 頂点データをVBOに転送する。
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

	// 頂点属性の構成を設定する。
	// 属性0は頂点位置。
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);

	// テクスチャ座標の構成を設定する。
	// 属性1はUV座標。
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	// VAOのバインドを解除する。
	glBindVertexArray(0);
}

void InitInterleaveRenderer() {


	shaderProgram = LoadShaders("passthrough.vert", "interleave.frag");
	videoshaderProgram = LoadShaders("passthrough.vert", "sbs_interleave.frag");

	// --- cache uniform locations (image) ---
	u_img_leftTexture = glGetUniformLocation(shaderProgram, "leftTexture");
	u_img_rightTexture = glGetUniformLocation(shaderProgram, "rightTexture");
	u_img_leftEnabled = glGetUniformLocation(shaderProgram, "leftEnabled");
	u_img_rightEnabled = glGetUniformLocation(shaderProgram, "rightEnabled");
	u_img_haba = glGetUniformLocation(shaderProgram, "haba");
	u_img_totalShift = glGetUniformLocation(shaderProgram, "totalShift");
	u_img_timeStep = glGetUniformLocation(shaderProgram, "timeStep");
	u_img_manualShift = glGetUniformLocation(shaderProgram, "manualShift");
	u_img_showAlignmentCenter = glGetUniformLocation(shaderProgram, "showAlignmentCenter");
	u_img_alignmentCenterSub = glGetUniformLocation(shaderProgram, "alignmentCenterSub");
	u_img_alignmentViewportHeight = glGetUniformLocation(shaderProgram, "alignmentViewportHeight");
	u_img_correctionThreshold = glGetUniformLocation(shaderProgram, "correctionThreshold");
	u_img_slantY = glGetUniformLocation(shaderProgram, "slantY");
	/*u_img_middleLinePx = glGetUniformLocation(shaderProgram, "middleLinePx");
	u_img_rgbGain = glGetUniformLocation(shaderProgram, "rgbGain");
	u_img_gamma = glGetUniformLocation(shaderProgram, "gammaValue");
	u_img_crosstalk = glGetUniformLocation(shaderProgram, "crosstalk");
	u_img_enableColorCorrection = glGetUniformLocation(shaderProgram, "enableColorCorrection");*/

	// --- cache uniform locations (video) ---
	u_vid_sbsTexture = glGetUniformLocation(videoshaderProgram, "sbsTexture");
	u_vid_leftEnabled = glGetUniformLocation(videoshaderProgram, "leftEnabled");
	u_vid_rightEnabled = glGetUniformLocation(videoshaderProgram, "rightEnabled");
	u_vid_haba = glGetUniformLocation(videoshaderProgram, "haba");
	u_vid_totalShift = glGetUniformLocation(videoshaderProgram, "totalShift");
	u_vid_timeStep = glGetUniformLocation(videoshaderProgram, "timeStep");
	u_vid_manualShift = glGetUniformLocation(videoshaderProgram, "manualShift");
	u_vid_showAlignmentCenter = glGetUniformLocation(videoshaderProgram, "showAlignmentCenter");
	u_vid_alignmentCenterSub = glGetUniformLocation(videoshaderProgram, "alignmentCenterSub");
	u_vid_alignmentViewportHeight = glGetUniformLocation(videoshaderProgram, "alignmentViewportHeight");
	u_vid_correctionThreshold = glGetUniformLocation(videoshaderProgram, "correctionThreshold");
	u_vid_slantY = glGetUniformLocation(videoshaderProgram, "slantY");
	/*u_vid_middleLinePx = glGetUniformLocation(videoshaderProgram, "middleLinePx");
	u_vid_rgbGain = glGetUniformLocation(videoshaderProgram, "rgbGain");
	u_vid_gamma = glGetUniformLocation(videoshaderProgram, "gammaValue");
	u_vid_crosstalk = glGetUniformLocation(videoshaderProgram, "crosstalk");
	u_vid_enableColorCorrection = glGetUniformLocation(videoshaderProgram, "enableColorCorrection");*/

	auto warnIfMissing = [](const char* name, GLint loc) {
		if (loc < 0) printf("[WARN] uniform not found: %s\n", name);
	};
	warnIfMissing("leftTexture", u_img_leftTexture);
	warnIfMissing("rightTexture", u_img_rightTexture);
	warnIfMissing("haba", u_img_haba);
	warnIfMissing("totalShift", u_img_totalShift);
	warnIfMissing("timeStep", u_img_timeStep);
	warnIfMissing("manualShift", u_img_manualShift);
	/*warnIfMissing("middleLinePx", u_img_middleLinePx);
	warnIfMissing("rgbGain", u_img_rgbGain);
	warnIfMissing("gammaValue", u_img_gamma);
	warnIfMissing("crosstalk", u_img_crosstalk);
	warnIfMissing("enableColorCorrection", u_img_enableColorCorrection);*/

	warnIfMissing("sbsTexture", u_vid_sbsTexture);
	warnIfMissing("haba", u_vid_haba);
	warnIfMissing("totalShift", u_vid_totalShift);
	warnIfMissing("timeStep", u_vid_timeStep);
	warnIfMissing("manualShift", u_vid_manualShift);
	warnIfMissing("video slantY", u_vid_slantY);
	/*warnIfMissing("middleLinePx", u_vid_middleLinePx);
	warnIfMissing("rgbGain", u_vid_rgbGain);
	warnIfMissing("gammaValue", u_vid_gamma);
	warnIfMissing("crosstalk", u_vid_crosstalk);
	warnIfMissing("enableColorCorrection", u_vid_enableColorCorrection);
*/
	setupQuad();
}

// 左右の静止画をサブピクセル単位で交互に描画する。
// 静止画用シェーダーの描画処理。
void renderInterleavedImage() {
	glUseProgram(shaderProgram); // 静止画用シェーダーを使用する。
	glBindVertexArray(quadVAO);

	// 追跡位置とバリア幅から位相補正を計算する。
	// 従来のステンシル計算と同じ基準を使う。
	int totalShift = MiddleLine - 48 * haba;

	// 描画パラメーターをuniformへ渡す。
	// 左右画像のテクスチャを設定する。
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imageL);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, imageR);
	glUniform1i(u_img_leftTexture, 0);
	glUniform1i(u_img_rightTexture, 1);

	glUniform1f(u_img_haba, (float)haba);
	glUniform1f(u_img_totalShift, (float)totalShift);
	glUniform1i(u_img_timeStep, kk);
	glUniform1i(u_img_leftEnabled, eyeleft);
	glUniform1i(u_img_rightEnabled, eyeright);
	glUniform1f(u_img_manualShift, (float)SHIFT);
	glUniform1i(u_img_showAlignmentCenter, showAlignmentCenter ? 1 : 0);
	glUniform1f(u_img_alignmentCenterSub, (float)MiddleLine);
	glUniform1f(u_img_alignmentViewportHeight, (float)IM_H);
	glUniform1f(u_img_correctionThreshold, barrierThresholdStep / 20.0f);
	if (u_img_slantY >= 0)
		glUniform1f(u_img_slantY, barrierSlantY);
	// 色補正uniformは現在無効。
	//glUniform1f(u_img_middleLinePx, (float)MiddleLine / 3.0f);
	//glUniform3f(u_img_rgbGain, colorGainR, colorGainG, colorGainB);
	//glUniform1f(u_img_gamma, colorGamma);
	//glUniform1f(u_img_crosstalk, crosstalkFactor);
	//glUniform1i(u_img_enableColorCorrection, colorCorrectionEnabled ? 1 : 0);
	// 四角形を描画する。
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// 描画状態を解除する。
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(0);
}

// 左右分割動画をサブピクセル単位で交互に描画する。
// 動画用シェーダーの描画処理。
void renderInterleavedVideo(GLuint videoTexID) {
	if (videoTexID == 0) return;

	glUseProgram(videoshaderProgram);
	glBindVertexArray(quadVAO);

	// 追跡位置とバリア幅から位相補正を計算する。
	int totalShift = MiddleLine - 48 * haba;

	// 描画パラメーターをuniformへ渡す。
	// 左右分割動画のテクスチャを設定する。
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, videoTexID);
	glUniform1i(u_vid_sbsTexture, 0);

	glUniform1f(u_vid_haba, (float)haba);
	glUniform1f(u_vid_totalShift, (float)totalShift);
	glUniform1i(u_vid_timeStep, kk);
	glUniform1i(u_vid_leftEnabled, eyeleft);
	glUniform1i(u_vid_rightEnabled, eyeright);
	glUniform1f(u_vid_manualShift, (float)SHIFT);
	glUniform1i(u_vid_showAlignmentCenter, showAlignmentCenter ? 1 : 0);
	glUniform1f(u_vid_alignmentCenterSub, (float)MiddleLine);
	glUniform1f(u_vid_alignmentViewportHeight, (float)IM_H);
	glUniform1f(u_vid_correctionThreshold, barrierThresholdStep / 20.0f);

	// 旧方式の斜め座標W + Hに対応する係数。
	if (u_vid_slantY >= 0)
		glUniform1f(u_vid_slantY, barrierSlantY);

	// 色補正uniformは現在無効。
	//glUniform1f(u_vid_middleLinePx, (float)MiddleLine / 3.0f);
	//glUniform3f(u_vid_rgbGain, colorGainR, colorGainG, colorGainB);
	//glUniform1f(u_vid_gamma, colorGamma);
	//glUniform1f(u_vid_crosstalk, crosstalkFactor);
	//glUniform1i(u_vid_enableColorCorrection, colorCorrectionEnabled ? 1 : 0);
	// 四角形を描画する。
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// 描画状態を解除する。
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(0);
}
