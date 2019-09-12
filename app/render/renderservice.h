#ifndef RENDERSERVICE_H
#define RENDERSERVICE_H


class RenderService
{
public:
  RenderService();

  static RenderService* CreateInstance();
  static RenderService* GetInstance();
  static RenderService* DestroyInstance();

private:
  static RenderService* render_service_;

};

#endif // RENDERSERVICE_H
