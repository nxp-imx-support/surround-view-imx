#!/bin/sh
cd /root/SV3D/
export LIBCAMERA_PIPELINES_MATCH_LIST='nxp/neo,imx8-isi,simple'
./SV3D-Wayland capture
