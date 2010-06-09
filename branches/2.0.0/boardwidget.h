/*
    mugo, sgf editor.
    Copyright (C) 2009-2010 nsase.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QGraphicsView>
#include "godata.h"


class SgfDocument;

namespace Ui {
    class BoardWidget;
}

/**
  BoardWidget
*/
class BoardWidget : public QGraphicsView {
    Q_OBJECT
public:
    class TerritoryInfo{
        public:
            TerritoryInfo() : color(Go::empty), territory(Go::empty){}

            bool isStone() const{ return color != Go::empty; }
            bool isBlack() const{ return color == Go::black; }
            bool isWhite() const{ return color == Go::white; }
            bool isTerritory() const{ return territory != Go::empty; }
            bool isBlackTerritory() const{ return territory != Go::black; }
            bool isWhiteTerritory() const{ return territory != Go::white; }

            Go::Color color;
            Go::Color territory;
            QGraphicsItem* stone;
    };

    BoardWidget(SgfDocument* doc, QWidget *parent = 0);
    ~BoardWidget();

    SgfDocument* document(){ return document_; }

    void setCurrentGame(Go::NodePtr node);
    void setCurrentNode(Go::NodePtr node);
    void forward();
    void back();

    Go::NodePtr getCurrentGame(){ return currentGame; }

    QString getCoordinateString(Go::NodePtr node, bool showI) const;

signals:
    void currentGameChanged(Go::NodePtr currentGame);
    void currentNodeChanged(Go::NodePtr currentNode);

protected:
    void resizeEvent(QResizeEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void wheelEvent(QWheelEvent* e);
    void onLButtonDown(QMouseEvent* e);
    void onRButtonDown(QMouseEvent* e);

    void createBuffer(bool erase);
    void getStarPosition(QList<int>& xpos, QList<int>& ypos);
    void killStones(int x, int y);
    void killStones(char* buf);
    bool canKillStones(int x, int y, Go::Color color, char* buf);
    bool inBoard(Go::NodePtr node);

private slots:

private:
    Ui::BoardWidget *ui;
    SgfDocument* document_;
    Go::NodePtr  currentGame;
    Go::GameInformationPtr gameInformation;
    QGraphicsScene* scene;
    QGraphicsRectItem* board;
    QGraphicsRectItem* shadow;
    QList<QGraphicsLineItem*> hLines;
    QList<QGraphicsLineItem*> vLines;
    QList<QGraphicsEllipseItem*> stars;
    QList<QGraphicsItem*> stones;
    Go::NodePtr currentNode;
    Go::NodeList currentNodeList;
    QVector< QVector<TerritoryInfo> > boardBuffer;
};

#endif // BOARDWIDGET_H
