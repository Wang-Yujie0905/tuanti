#include "gamewidget.h"
#include <QApplication>
#include <QtGui>
#include <ctime>
#include <vector>
#include <cmath>
#include <cassert>
#include <conio.h>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <windows.h>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QDateTime>
#include <QSplashScreen>
#include <control.h>
#include <QGraphicsView>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GameWidget w;
    w.show();

    return a.exec();


}
