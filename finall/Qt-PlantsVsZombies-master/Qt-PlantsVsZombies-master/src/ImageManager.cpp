//
// Created by sun on 8/26/16.
//

#include "ImageManager.h"

ImageManager *gImageCache;

QPixmap ImageManager::load(const QString &path)
{
    if (pixmaps.find(path) == pixmaps.end())
        pixmaps.insert(path, QPixmap(":/images/" + path));
    return pixmaps[path];
}

void ImageManager::preload(const QList<QString> &paths)
{
    for (const QString &path : paths) {
        if (pixmaps.find(path) == pixmaps.end())
            pixmaps.insert(path, QPixmap(":/images/" + path));
    }
}

void InitImageManager()
{
    gImageCache = new ImageManager;
}

void DestoryImageManager()
{
    delete gImageCache;
}