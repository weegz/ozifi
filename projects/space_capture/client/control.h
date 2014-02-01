#pragma once

#include <QObject>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTime>
#include <QPoint>

#include "world.h"

class TControl: public QObject {
    Q_OBJECT
public:
    explicit TControl(TWorld* world);
public slots:
    void OnMouseEvent(QMouseEvent event, bool pressed);
    void OnMouseMove(QMouseEvent event);
    void OnResizeEvent(QResizeEvent event);
signals:
    void OnControl(Space::TControl control);
private:
    void timerEvent(QTimerEvent *);
private:
    TWorld* World;
    QTime LastSendControl;
    bool MousePressed;
    QPoint SelectionFrom;
};
