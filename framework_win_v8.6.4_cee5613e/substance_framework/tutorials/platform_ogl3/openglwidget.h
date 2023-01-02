/*************************************************************************
* ADOBE CONFIDENTIAL
* ___________________ *
*  Copyright 2020-2022 Adobe
*  All Rights Reserved.
* * NOTICE:  All information contained herein is, and remains
* the property of Adobe and its suppliers, if any. The intellectual
* and technical concepts contained herein are proprietary to Adobe
* and its suppliers and are protected by all applicable intellectual
* property laws, including trade secret and copyright laws.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe.
**************************************************************************/

#pragma once

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>

#include <string>
#include <fstream>

class OpenglWidget : public QOpenGLWidget {
public:
	OpenglWidget(QWidget* parent = nullptr)
		: currentTexture_(0)
		, QOpenGLWidget(parent) {

	}

	void pushResult(SubstanceAir::RenderResultImage* res) {
		results_.push_back(res);
	}

	void initializeGL() {
		vao = new QOpenGLVertexArrayObject;
		vao->create();
		vao->bind();

		static const char *vs =
			"#version 330\n"
			"const vec2 positionGen[3] = vec2[3](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));\n"
			"out vec2 texCoord;\n"
			"void main()\n"
			"{\n"
			"	 texCoord = positionGen[gl_VertexID];\n"
			"	 gl_Position = vec4(2.0*positionGen[gl_VertexID] - 1.0, 0.0, 1.0); \n"
			"}\n";

		static const char *fs =
			"#version 330\n"
			"in vec2 texCoord;\n"
			"uniform sampler2D tex;\n"
			"out vec4 ocolor0;\n"
			"void main()\n"
			"{\n"
			"	ocolor0 = texture(tex, texCoord);\n"
			"}\n";

		m_program = new QOpenGLShaderProgram(this);
		m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vs);
		m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fs);
		m_program->link();
	}

	void paintGL() {
		glClearColor(0.42f, 0.f, 0.42f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (!results_.empty()) {
			m_program->bind();
			glBindTexture(GL_TEXTURE_2D, results_[currentTexture_]->getTexture().textureName);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			m_program->release();
		}
	}

	void mouseReleaseEvent(QMouseEvent *)
	{
		currentTexture_ = currentTexture_ >= (unsigned int)results_.size()-1 ? 0 : currentTexture_ + 1;
		update();
	}

	typedef std::vector<SubstanceAir::RenderResultImage*> Results;
	Results results_;

	unsigned int currentTexture_;

	QOpenGLVertexArrayObject* vao;
	QOpenGLShaderProgram* m_program;
};