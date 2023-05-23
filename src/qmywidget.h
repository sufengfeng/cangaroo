#ifndef QMYWIDGET_H
#define QMYWIDGET_H

#include <QWidget>

class QMyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QMyWidget(QWidget* parent = nullptr);
public:
    QSize sizeHint() const
    {
        return size;
    }
    QSize size;

signals:

public slots:
};

#endif // QMYWIDGET_H
