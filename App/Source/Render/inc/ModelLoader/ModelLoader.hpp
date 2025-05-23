﻿/*
*
* Copyright 2017,2022 NXP
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#ifndef MODELLOADER_HPP_
#define MODELLOADER_HPP_

#include <string>
#include <vector>

#include "Material.hpp"
#include "VBO.hpp"

using namespace std;

#define CONFIG_MODEL_FILE "../Content/model.cfg"

typedef vector<Material*> MaterialList;
typedef MaterialList::iterator MaterialListIter;

typedef vector<VBO*> VBOList;
typedef VBOList::iterator VBOListIter;

class ModelLoader
{
protected:
	bool isInitialized;

	MaterialList materials;
	VBOList objects;

public:
	ModelLoader(void);
	~ModelLoader(void);

	bool Initialize(void);
	string GetModelFileName(void);
	void Draw(GLuint shader);

};

#endif /* MODELLOADER_HPP_ */

