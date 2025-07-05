/*
MIT License

Copyright (c) 2021-2022 L. E. Spalt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <vector>
#include <float.h>
#include <math.h>
#include "Overlay.h"
#include "iracing.h"
#include "Config.h"

class OverlayTrackMap : public Overlay
{
public:
    OverlayTrackMap()
        : Overlay("OverlayTrackMap")
    {}

protected:
    struct Point { float x; float y; };
    std::vector<Point> m_points;
    double             m_lastTime = 0.0;
    float              m_lastPct = 0.0f;
    bool               m_lapDone = false;
    bool               m_rotated = false;

    virtual float2 getDefaultSize()
    {
        return float2(200,200);
    }

    virtual void onEnable()
    {
        m_points.clear();
        m_lastTime = ir_SessionTime.getDouble();
        m_lastPct = ir_LapDistPct.getFloat();
        m_lapDone = false;
        m_rotated = false;
    }

    virtual void onDisable()
    {
        m_points.clear();
    }

    virtual void onUpdate()
    {
        double t = ir_SessionTime.getDouble();
        float dt = (float)(t - m_lastTime);
        m_lastTime = t;

        float yaw = ir_Yaw.getFloat();
        float speed = ir_Speed.getFloat();

        float dx = cosf(yaw) * speed * dt;
        float dy = sinf(yaw) * speed * dt;

        Point p = m_points.empty() ? Point{0,0} : m_points.back();
        p.x += dx;
        p.y += dy;
        m_points.push_back(p);

        float pct = ir_LapDistPct.getFloat();
        if( pct < m_lastPct && m_points.size() > 10 )
            m_lapDone = true;
        m_lastPct = pct;

        if( !m_lapDone )
            return;

        if( m_points.size() < 2 )
            return;

        if( !m_rotated )
        {
            float minx=FLT_MAX, miny=FLT_MAX, maxx=-FLT_MAX, maxy=-FLT_MAX;
            for( const Point& pt : m_points )
            {
                if( pt.x < minx ) minx = pt.x;
                if( pt.y < miny ) miny = pt.y;
                if( pt.x > maxx ) maxx = pt.x;
                if( pt.y > maxy ) maxy = pt.y;
            }
            if( (maxy-miny) > (maxx-minx) )
            {
                for( Point& pt : m_points )
                {
                    float nx = -pt.y;
                    float ny = pt.x;
                    pt.x = nx;
                    pt.y = ny;
                }
            }
            m_rotated = true;
        }

        float minx=FLT_MAX, miny=FLT_MAX, maxx=-FLT_MAX, maxy=-FLT_MAX;
        for( const Point& pt : m_points )
        {
            if( pt.x < minx ) minx = pt.x;
            if( pt.y < miny ) miny = pt.y;
            if( pt.x > maxx ) maxx = pt.x;
            if( pt.y > maxy ) maxy = pt.y;
        }
        const float scale = 0.9f * std::min((float)m_width/(maxx-minx+1e-3f), (float)m_height/(maxy-miny+1e-3f));
        const float offx = (float)m_width*0.5f - (minx+maxx)*0.5f*scale;
        const float offy = (float)m_height*0.5f - (miny+maxy)*0.5f*scale;

        Microsoft::WRL::ComPtr<ID2D1PathGeometry1> path;
        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
        m_d2dFactory->CreatePathGeometry(&path);
        path->Open(&sink);
        sink->BeginFigure(float2(m_points[0].x*scale+offx, m_points[0].y*scale+offy), D2D1_FIGURE_BEGIN_HOLLOW);
        for(size_t i=1;i<m_points.size();++i)
            sink->AddLine(float2(m_points[i].x*scale+offx, m_points[i].y*scale+offy));
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        sink->Close();

        m_renderTarget->BeginDraw();
        const float thickness = g_cfg.getFloat(m_name, "line_thickness", 2.0f);
        m_brush->SetColor(g_cfg.getFloat4(m_name, "line_col", float4(1,1,1,1)) );
        m_renderTarget->DrawGeometry(path.Get(), m_brush.Get(), thickness);
        m_renderTarget->EndDraw();
    }
};

