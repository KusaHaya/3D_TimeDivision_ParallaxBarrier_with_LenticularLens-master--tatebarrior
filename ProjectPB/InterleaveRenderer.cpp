#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <stdio.h>
#include "AppState.h"
#include "ShaderLoader.h"
#include "InterleaveRenderer.h"

namespace {

// 静止画と動画で共通のシェーダー（interleave.frag）。
GLuint interleaveProgram = 0;

// uniform location cache
struct InterleaveUniforms {
	GLint leftTexture = -1;
	GLint rightTexture = -1;
	GLint sideBySide = -1;
	GLint leftEnabled = -1;
	GLint rightEnabled = -1;
	GLint haba = -1;
	GLint totalShift = -1;
	GLint timeStep = -1;
	GLint manualShift = -1;
	GLint slantY = -1;
	GLint correctionThreshold = -1;
	GLint showAlignmentCenter = -1;
	GLint alignmentCenterSub = -1;
	GLint alignmentViewportHeight = -1;
};
InterleaveUniforms uniforms;

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

	// 属性0は頂点位置。
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);

	// 属性1はUV座標。
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	// VAOのバインドを解除する。
	glBindVertexArray(0);
}

// 静止画・動画共通の描画。sideBySide=true なら leftTex の左右半分を左右の目に使う。
void renderInterleaved(GLuint leftTex, GLuint rightTex, bool sideBySide) {
	glUseProgram(interleaveProgram);
	glBindVertexArray(quadVAO);

	// 追跡位置とバリア幅から位相補正を計算する。
	// 従来のステンシル計算と同じ基準を使う。
	int totalShift = MiddleLine - 48 * haba;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, leftTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, rightTex);
	glUniform1i(uniforms.leftTexture, 0);
	glUniform1i(uniforms.rightTexture, 1);
	glUniform1i(uniforms.sideBySide, sideBySide ? 1 : 0);

	glUniform1f(uniforms.haba, (float)haba);
	glUniform1f(uniforms.totalShift, (float)totalShift);
	glUniform1i(uniforms.timeStep, kk);
	glUniform1i(uniforms.leftEnabled, eyeleft);
	glUniform1i(uniforms.rightEnabled, eyeright);
	glUniform1f(uniforms.manualShift, (float)SHIFT);
	glUniform1i(uniforms.showAlignmentCenter, showAlignmentCenter ? 1 : 0);
	glUniform1f(uniforms.alignmentCenterSub, (float)MiddleLine);
	glUniform1f(uniforms.alignmentViewportHeight, (float)IM_H);
	glUniform1f(uniforms.correctionThreshold, barrierThresholdStep / 20.0f);
	glUniform1f(uniforms.slantY, barrierSlantY);

	// 四角形を描画する。
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// 描画状態を解除する。
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(0);
}

}

bool InitInterleaveRenderer() {
	interleaveProgram = LoadShaders("passthrough.vert", "interleave.frag");
	if (interleaveProgram == 0) {
		printf("[FATAL] interleave.frag could not be loaded. Check that the shader files are next to the exe.\n");
		return false;
	}

	const GLuint p = interleaveProgram;
	uniforms.leftTexture = glGetUniformLocation(p, "leftTexture");
	uniforms.rightTexture = glGetUniformLocation(p, "rightTexture");
	uniforms.sideBySide = glGetUniformLocation(p, "sideBySide");
	uniforms.leftEnabled = glGetUniformLocation(p, "leftEnabled");
	uniforms.rightEnabled = glGetUniformLocation(p, "rightEnabled");
	uniforms.haba = glGetUniformLocation(p, "haba");
	uniforms.totalShift = glGetUniformLocation(p, "totalShift");
	uniforms.timeStep = glGetUniformLocation(p, "timeStep");
	uniforms.manualShift = glGetUniformLocation(p, "manualShift");
	uniforms.slantY = glGetUniformLocation(p, "slantY");
	uniforms.correctionThreshold = glGetUniformLocation(p, "correctionThreshold");
	uniforms.showAlignmentCenter = glGetUniformLocation(p, "showAlignmentCenter");
	uniforms.alignmentCenterSub = glGetUniformLocation(p, "alignmentCenterSub");
	uniforms.alignmentViewportHeight = glGetUniformLocation(p, "alignmentViewportHeight");

	// These drive the barrier pattern; a missing one silently changes the image.
	struct { const char* name; GLint location; } required[] = {
		{"leftTexture", uniforms.leftTexture}, {"rightTexture", uniforms.rightTexture},
		{"sideBySide", uniforms.sideBySide}, {"haba", uniforms.haba},
		{"totalShift", uniforms.totalShift}, {"timeStep", uniforms.timeStep},
		{"manualShift", uniforms.manualShift}, {"slantY", uniforms.slantY},
		{"leftEnabled", uniforms.leftEnabled}, {"rightEnabled", uniforms.rightEnabled},
	};
	bool ok = true;
	for (const auto& u : required) {
		if (u.location < 0) {
			printf("[FATAL] uniform not found in interleave.frag: %s\n", u.name);
			ok = false;
		}
	}
	if (!ok) return false;

	setupQuad();
	return true;
}

void renderInterleavedImage() {
	renderInterleaved(imageL, imageR, false);
}

void renderInterleavedVideo(GLuint videoTexID) {
	if (videoTexID == 0) return;
	renderInterleaved(videoTexID, 0, true);
}
