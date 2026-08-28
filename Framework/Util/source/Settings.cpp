/*
 * Copyright 2017, 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Settings.hpp"

#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

int Settings::ReadXML(const char* filename)
{
    xmlDocPtr pdoc;
    xmlNodePtr pnode;

    std::string filePath = AssetManager::GetPath(std::string(filename));
    pdoc = xmlReadFile(filePath.c_str(), NULL, 0);

    if (pdoc == NULL) {
        LogError("Cannot open file: %s", filePath.c_str());
        return (-1);
    }

    pnode = xmlDocGetRootElement(pdoc);

    if (pnode == NULL) {
        LogError("Empty file: %s", filePath.c_str());
        xmlFreeDoc(pdoc);
        return (-1);
    }

    for (pnode = pnode->children; pnode != NULL; pnode = pnode->next) {
        if (pnode->type == XML_ELEMENT_NODE) {
            xmlNodePtr pchildren = pnode->xmlChildrenNode;
            for (pchildren = pnode->children; pchildren != NULL; pchildren = pchildren->next) {
                if (pchildren->type == XML_ELEMENT_NODE) {
                    xmlChar* ret_val = xmlNodeGetContent(pchildren);
                    if (ret_val != NULL) {
                        if (SetParam(GetParam((const char*)pchildren->name), (const char*)ret_val) == -1) {
                            xmlFreeDoc(pdoc);
                            return (-1);
                        }
                    }
                }
            }
        }
    }
    xmlFreeDoc(pdoc);
    return (0);
}

int Settings::SetParam(int num, const char* val)
{
    int ret_val = 0;
    switch (num) {
    case 0:
        cameraInputs = std::string(val) + "/";
        break;
    case 1:
        cameraModels = std::string(val) + "/";
        break;
    case 2:
        templateFiles = std::string(val) + "/";
        break;
    case 3:
        camerasCount = atoi(val);
        if ((camerasCount < 1) || (camerasCount > 4)) {
            LogError("Camera numbers must be in [1, 4]");
            return (-1);
        }
        break;
    case 4:
        ret_val = ReadUInt(val, &displayHeight);
        break;
    case 5:
        ret_val = ReadUInt(val, &displayWidth);
        break;
    case 6:
        ReadBool(val, &showDebug);
        break;
    case 7:
        ret_val = ReadUInt(val, &gridAngles);
        break;
    case 8:
        ret_val = ReadUInt(val, &gridStartAngle);
        break;
    case 9:
        ret_val = ReadUInt(val, &gridPointsZCount);
        break;
    case 10:
        ret_val = ReadFloat(val, &gridStepX);
        break;
    case 11:
        ret_val = ReadFloat(val, &bowlRadius);
        break;
    case 12:
        ret_val = ReadFloat(val, &smoothAngle);
        break;
    case 13:
        keyboard = std::string(val);
        break;
    case 14:
        mouse = std::string(val);
        break;
    case 15:
        display = std::string(val);
        break;
    case 16:
        ret_val = ReadFloat(val, &modelScale[0]);
        break;
    case 17:
        ret_val = ReadFloat(val, &modelScale[1]);
        break;
    case 18:
        ret_val = ReadFloat(val, &modelScale[2]);
        break;
    case 19:
        ret_val = ReadUInt(val, &maxFPS);
        break;
    case 20:
        ReadBool(val, &objDetEnable);
        break;
    case 21:
        ret_val = ReadUInt(val, &maxIPS);
        break;
    case 22:
        ReadBool(val, &fullscreen);
        break;
    case 23:
        ret_val = ReadUInt(val, &ecRefreshRate);
        break;
    case 24:
        ReadBool(val, &dewarp);
        break;
    case 100:
    case 101:
    case 102:
    case 103:
        ret_val = ReadCamera(val, num - 100, &cameras[num - 100]);
        break;
    default:
        LogError("Too much parameters in xml file");
        ret_val = -1;
        break;
    }
    return (ret_val);
}

int Settings::ReadUInt(const char* src, int* dst)
{
    *dst = atoi(src);
    if (*dst < 0) {
        LogError("All parameters must be a positive number");
        return (-1);
    }
    return (0);
}

int Settings::ReadFloat(const char* src, float* dst)
{
    *dst = (float)atof(src);
    if (*dst < 0.0) {
        LogError("All parameters must be a positive number");
        return (-1);
    }
    return (0);
}

void Settings::ReadBool(const char* src, bool* dst)
{
    int intval = atoi(src);
    *dst = (intval != 0);
}

int Settings::ReadCamera(const char* src, int index, CamParam* dst)
{
    std::stringstream str;
    str << src;

    std::string line;
    std::vector<std::string> lines;

    while (std::getline(str, line)) {
        // Remove trailing spaces
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(),
            line.end());

        // Remove leading spaces
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));

        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    if (lines.size() < 8) {
        LogError("Missing camera arguments");
        return -1;
    }

    dst->height = std::stoi(lines[0]);
    dst->width = std::stoi(lines[1]);
    dst->sf = std::stof(lines[2]);
    dst->roi = std::stoi(lines[3]);
    dst->cntr_min_size = std::stoi(lines[4]);
    dst->chessboard_num = std::stoi(lines[5]);
    dst->source = lines[6];
    dst->format = lines[7];

    return 0;
}

void Settings::PrintParam(void)
{
    LogInfo("Path to camera calibration static images %s", cameraInputs.c_str());
    LogInfo("Path to camera models %s", cameraModels.c_str());
    LogInfo("Path to templates %s", templateFiles.c_str());
    LogInfo("Number of cameras %d", camerasCount);
    for (int i = 0; i < 4; i++) {
        LogInfo("Camera %d", i + 1);
        LogInfo("\tResolution %dx%d", cameras[i].height, cameras[i].width);
        LogInfo("\tDefisheye scale factor sf = %f", cameras[i].sf);
        LogInfo("\tROI in which contours will be searched (%% of image height) roi = %d", cameras[i].roi);
        LogInfo("\tContours min size = %d", cameras[i].cntr_min_size);
        LogInfo("\tChessboard images number = %d", cameras[i].chessboard_num);
        LogInfo("\tSource = %s", cameras[i].source.c_str());
    }
    LogInfo("Display resolution %dx%d", displayHeight, displayWidth);
    LogInfo("Show debug info %d", showDebug);
    LogInfo("Max FPS = %d", maxFPS);
    LogInfo("Fullscreen = %d", fullscreen);
    LogInfo("Enable Object Detection = %d", objDetEnable);
    LogInfo("Max IPS = %d", maxIPS);
    LogInfo("Exposure Correction refresh rate = %d", ecRefreshRate);
    LogInfo("Grig parameters");
    LogInfo("\tAngles number %d", gridAngles);
    LogInfo("\tStart angle %d", gridStartAngle);
    LogInfo("\tNumber of grid points in z axis %d", gridPointsZCount);
    LogInfo("\tStep in x axis %f", gridStepX);
    LogInfo("\tRadius of 3D bowl %f", bowlRadius);
    LogInfo("Mask angle of smoothing %f", smoothAngle);
    LogInfo("Keyboard events %s", keyboard.c_str());
    LogInfo("Mouse events %s", mouse.c_str());
    LogInfo("Display file %s", display.c_str());
    LogInfo("Car model scale (%f,%f,%f)", modelScale[0], modelScale[1], modelScale[2]);
}

int Settings::GetTmpMaxVal(const char* filename, int* val)
{
    *val = 0;
    struct stat st;
    std::string refPointsTxt = AssetManager::GetPath(templateFiles + (std::string)filename);
    if (stat(refPointsTxt.c_str(), &st) != 0) {
        LogError("File %s not found", refPointsTxt.c_str());
        return (-1);
    }
    int x, y;

    std::ifstream ifsRef(refPointsTxt.c_str());
    while (ifsRef >> x >> y) {
        if (x > *val) {
            *val = x;
        }
    }
    ifsRef.close();
    return (0);
}

int Settings::GetParam(const char* name)
{
    LogDebug("Getting parameter number for: %s", name);
    int value = -1;
    if (strcmp(name, "camera_inputs") == 0) {
        value = 0;
    } else if (strcmp(name, "camera_models") == 0) {
        value = 1;
    } else if (strcmp(name, "template") == 0) {
        value = 2;
    } else if (strcmp(name, "number") == 0) {
        value = 3;
    } else if (strcmp(name, "height") == 0) {
        value = 4;
    } else if (strcmp(name, "width") == 0) {
        value = 5;
    } else if (strcmp(name, "show_debug_img") == 0) {
        value = 6;
    } else if (strcmp(name, "angles") == 0) {
        value = 7;
    } else if (strcmp(name, "start_angle") == 0) {
        value = 8;
    } else if (strcmp(name, "nop_z") == 0) {
        value = 9;
    } else if (strcmp(name, "step_x") == 0) {
        value = 10;
    } else if (strcmp(name, "radius") == 0) {
        value = 11;
    } else if (strcmp(name, "smooth_angle") == 0) {
        value = 12;
    } else if (strcmp(name, "keyboard") == 0) {
        value = 13;
    } else if (strcmp(name, "mouse") == 0) {
        value = 14;
    } else if (strcmp(name, "display") == 0) {
        value = 15;
    } else if (strcmp(name, "x_scale") == 0) {
        value = 16;
    } else if (strcmp(name, "y_scale") == 0) {
        value = 17;
    } else if (strcmp(name, "z_scale") == 0) {
        value = 18;
    } else if (strcmp(name, "max_fps") == 0) {
        value = 19;
    } else if (strcmp(name, "enable_detection") == 0) {
        value = 20;
    } else if (strcmp(name, "max_ips") == 0) {
        value = 21;
    } else if (strcmp(name, "fullscreen") == 0) {
        value = 22;
    } else if (strcmp(name, "ec_refresh_rate") == 0) {
        value = 23;
    } else if (strcmp(name, "dewarp") == 0) {
        value = 24;
    } else if (strcmp(name, "camera1") == 0) {
        value = 100;
    } else if (strcmp(name, "camera2") == 0) {
        value = 101;
    } else if (strcmp(name, "camera3") == 0) {
        value = 102;
    } else if (strcmp(name, "camera4") == 0) {
        value = 103;
    } else {
        value = -1;
    }
    return (value);
}
