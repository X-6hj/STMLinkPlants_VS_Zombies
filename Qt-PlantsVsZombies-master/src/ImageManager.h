//
// Created by sun on 8/26/16.
//

#ifndef PLANTS_VS_ZOMBIES_IMAGEMANAGER_H
#define PLANTS_VS_ZOMBIES_IMAGEMANAGER_H


#include <QtGui>

class ImageManager
{
public:
    QPixmap load(const QString &path);
    void preload(const QList<QString> &paths);  // 预加载常用图片，减少首帧卡顿

private:
    QMap<QString, QPixmap> pixmaps;
};

extern ImageManager *gImageCache;

void InitImageManager();
void DestoryImageManager();

#endif //PLANTS_VS_ZOMBIES_IMAGEMANAGER_H
