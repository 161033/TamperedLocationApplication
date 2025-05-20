#pragma once
#include "sdl_view.h"
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
class XSDL :public XVideoView
{
public:
    void Close() override;

    /// ��ʼ����Ⱦ����
    bool Init(int w, int h,
        Format fmt = RGBA) override;

    /// ��Ⱦͼ��
    bool Draw(const unsigned  char* data,
        int linesize = 0) override;
    bool Draw(
        const unsigned  char* y, int y_pitch,
        const unsigned  char* u, int u_pitch,
        const unsigned  char* v, int v_pitch
    ) override;
    bool IsExit() override;

private:
    SDL_Window* win_ = nullptr;
    SDL_Renderer* render_ = nullptr;
    SDL_Texture* texture_ = nullptr;
};