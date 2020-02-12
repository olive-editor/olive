/*
Copyright (c) 2009-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <map>
#include <memory>
#include <unordered_map>
#include <OpenImageIO/ustring.h>
#include <OSL/oslexec.h>

OSL_NAMESPACE_ENTER

class SimpleRenderer : public RendererServices
{
public:
    // Just use 4x4 matrix for transformations
    typedef Matrix44 Transformation;

    SimpleRenderer ();
    ~SimpleRenderer () { }

    virtual int supports (string_view feature) const;
    virtual bool get_matrix (ShaderGlobals *sg, Matrix44 &result,
                             TransformationPtr xform,
                             float time);
    virtual bool get_matrix (ShaderGlobals *sg, Matrix44 &result,
                             ustring from, float time);
    virtual bool get_matrix (ShaderGlobals *sg, Matrix44 &result,
                             TransformationPtr xform);
    virtual bool get_matrix (ShaderGlobals *sg, Matrix44 &result,
                             ustring from);
    virtual bool get_inverse_matrix (ShaderGlobals *sg, Matrix44 &result,
                                     ustring to, float time);

    void name_transform (const char *name, const Transformation &xform);

    virtual bool get_array_attribute (ShaderGlobals *sg, bool derivatives, 
                                      ustring object, TypeDesc type, ustring name,
                                      int index, void *val );
    virtual bool get_attribute (ShaderGlobals *sg, bool derivatives, ustring object,
                                TypeDesc type, ustring name, void *val);
    virtual bool get_userdata (bool derivatives, ustring name, TypeDesc type, 
                               ShaderGlobals *sg, void *val);

    // Super simple camera and display parameters.  Many options not
    // available, no motion blur, etc.
    void camera_params (const Matrix44 &world_to_camera, ustring projection,
                        float hfov, float hither, float yon,
                        int xres, int yres);
                        
private:
    // Camera parameters
    Matrix44 m_world_to_camera;
    ustring m_projection;
    float m_fov, m_pixelaspect, m_hither, m_yon;
    float m_shutter[2];
    float m_screen_window[4];
    int m_xres, m_yres;

    // Named transforms
    typedef std::map <ustring, std::shared_ptr<Transformation> > TransformMap;
    TransformMap m_named_xforms;

    // Attribute and userdata retrieval -- for fast dispatch, use a hash
    // table to map attribute names to functions that retrieve them. We
    // imagine this to be fairly quick, but for a performance-critical
    // renderer, we would encourage benchmarking various methods and
    // alternate data structures.
    typedef bool (SimpleRenderer::*AttrGetter)(ShaderGlobals *sg, bool derivs,
                                               ustring object, TypeDesc type,
                                               ustring name, void *val);
    typedef std::unordered_map<ustring, AttrGetter, ustringHash> AttrGetterMap;
    AttrGetterMap m_attr_getters;

    // Attribute getters
    bool get_osl_version (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_resolution (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_projection (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_fov (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_pixelaspect (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_clip (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_clip_near (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_clip_far (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_shutter (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_shutter_open (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_shutter_close (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);
    bool get_camera_screen_window (ShaderGlobals *sg, bool derivs, ustring object,
                         TypeDesc type, ustring name, void *val);

};

OSL_NAMESPACE_EXIT
