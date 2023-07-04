/*

  Copyright (c) 2015, 2016 Hubert Denkmair <hubert@denkmair.de>

  This file is part of cangaroo.

  cangaroo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  cangaroo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cangaroo.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "mainwindow.h"
#include <QApplication>
#include <QtDebug>
#include "windows.h"
#define MAX_PROGRESS 100
#define PROGRESS_BAR_LENGTH 50

void print_percentage(int percentage)
{
    int filled_length = (percentage * PROGRESS_BAR_LENGTH) / MAX_PROGRESS;

    printf("\r进度：[%3d%%] [", percentage);

    for(int i = 0; i < filled_length; i++)
    {
        printf("*");
    }

    for(int i = filled_length; i < PROGRESS_BAR_LENGTH; i++)
    {
        printf(" ");
    }

    printf("]");
    fflush(stdout);
}


int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    //    w.show();
    w.ShowTerminal();
    return a.exec();
}
