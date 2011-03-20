/*
    mugo, sgf editor.
    Copyright (C) 2009-2011 nsase.

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
#include <QDebug>
#include <QMouseEvent>
#include "mugoapp.h"
#include "boardwidget.h"
#include "sgfcommand.h"

/**
  Constructor
*/
BoardWidget::BoardWidget(SgfDocument* doc, QWidget* parent) :
    QGraphicsView(parent),
    document_(doc),
    editMode_(eAlternateMove)
{
    connect(document_, SIGNAL(nodeAdded(Go::NodePtr)), SLOT(on_document_nodeAdded(Go::NodePtr)));
    connect(document_, SIGNAL(nodeDeleted(Go::NodePtr)), SLOT(on_document_nodeDeleted(Go::NodePtr)));

    setScene( new QGraphicsScene(this) );
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    // create board
    shadow = scene()->addRect(0, 0, 1, 1, QPen(Qt::NoPen), QBrush(SHADOW_COLOR));
    board  = scene()->addRect(0, 0, 1, 1, QPen(Qt::NoPen), QBrush(BOARD_COLOR));
    shadow->setZValue(0);
    board->setZValue(1);

    // set current game
    setGame(document_->gameList.front());
}

/**
  resize event
*/
void BoardWidget::resizeEvent(QResizeEvent*){
    setItemsPosition();
}

/**
  mouse move event
*/
void BoardWidget::mouseMoveEvent(QMouseEvent* e){
}

/**
  mouse button up event
*/
void BoardWidget::mouseReleaseEvent(QMouseEvent* e){
    if (e->button() == Qt::LeftButton)
        onLButtonUp(e);
    else if (e->button() == Qt::RightButton)
        onRButtonUp(e);
}

/**
  mouse wheel event
*/
void BoardWidget::wheelEvent(QWheelEvent* e){
    if (e->delta() > 0)
        back();
    else if (e->delta() < 0)
        forward();
}

/**
  mouse left button up event
*/
void BoardWidget::onLButtonUp(QMouseEvent* e){
    int sgfX, sgfY;
    if (viewToSgfCoordinate(e->x(), e->y(), sgfX, sgfY) == false)
        return;

    if (editMode_ == eAlternateMove)
        alternateMove(sgfX, sgfY);
}

/**
  mouse right button up event
*/
void BoardWidget::onRButtonUp(QMouseEvent* e){
}

/**
  set current game
*/
bool BoardWidget::setGame(const Go::NodePtr& game){
    // if game is not found in game list, return false
    Go::NodeList::const_iterator iter = qFind(document_->gameList.begin(), document_->gameList.end(), game);
    if (iter == document_->gameList.end())
        return false;

    // change current game
    currentGame_ = game;
    rootInformation_ = currentGame_->information();

    // create vertical lines
    qDeleteAll(vLines);
    vLines.clear();
    for(int i=0; i<xsize(); ++i){
        QGraphicsLineItem* line = scene()->addLine(0, 0, 0, 0);
        line->setZValue(2);
        vLines.push_back(line);
    }

    // create horizontal lines
    qDeleteAll(hLines);
    hLines.clear();
    for(int i=0; i<ysize(); ++i){
        QGraphicsLineItem* line = scene()->addLine(0, 0, 0, 0);
        line->setZValue(2);
        hLines.push_back(line);
    }

    // create buffer
    data.resize(ysize());
    buffer.resize(ysize());
    for (int i=0; i<ysize(); ++i){
        data[i].resize(xsize());
        buffer[i].resize(xsize());
    }

    // emit gameChanged
    emit gameChanged(currentGame_);

    // change current node
    setNode(currentGame_);

    return true;
}

/**
  set game information
*/
bool BoardWidget::setInformation(const Go::InformationPtr& information){
    // change game information, and emit currentGameInformationChanged
    currentInformation_ = information;
    emit informationChanged(currentInformation_);
    return true;
}

/**
  set current node
*/
bool BoardWidget::setNode(const Go::NodePtr& node){
    // if node isn't in the node list, create node list
    Go::NodeList::const_iterator iter = qFind(currentNodeList_.begin(), currentNodeList_.end(), node);
    if (iter == currentNodeList_.end())
        createNodeList(node);

    // change game information
    if (node->information())
        setInformation(node->information());

    // change current node, and emit currentNodeChanged
    currentNode_ = node;
    emit nodeChanged(currentGame_);

    // create buffer
    createBoardBuffer();

    return true;
}

/**
  set scene items position in board area
*/
void BoardWidget::setItemsPosition(){
    setItemsPosition(geometry());
}

/**
  set scene items position in rect area
*/
void BoardWidget::setItemsPosition(const QRectF& rect){
    // calculate board size
    int width  = rect.width();
    int height = rect.height();

    int gridW = width  / (xsize() + 1);
    int gridH = height / (ysize() + 1);
    int gridSize = qMin(gridW, gridH);
    int w = gridSize * (xsize() - 1);
    int h = gridSize * (ysize() - 1);

    int x = (rect.width() - w) / 2;
    int y = (rect.height() - h) / 2;
    int margin = gridSize * 0.6;
    QRect boardRect(QPoint(x-margin, y-margin), QPoint(x+w+margin, y+h+margin));

    // set position of shadow.
    shadow->setRect(boardRect.left()+3, boardRect.top()+3, boardRect.width(), boardRect.height());

    // set position of board.
    board->setRect(boardRect);

    // set scene rect
    QRect r = boardRect;
    r.setRight(shadow->rect().right());
    r.setBottom(shadow->rect().bottom());

    // move items position to current board size
    setVLinesPosition(x, y, gridSize);
    setHLinesPosition(x, y, gridSize);

    // move all datas position to current board size
    setDataPosition();

    //
    scene()->setSceneRect(r);
}

/**
  set vertical lines position
*/
void BoardWidget::setVLinesPosition(int x, int y, int gridSize){
    int len = gridSize * (ysize() - 1);
    for (int i=0; i<vLines.size(); ++i){
        vLines[i]->setLine(x, y, x, y+len);
        x += gridSize;
    }
}

/**
  set horizontal lines position
*/
void BoardWidget::setHLinesPosition(int x, int y, int gridSize){
    int len = gridSize * (xsize() - 1);
    for (int i=0; i<hLines.size(); ++i){
        hLines[i]->setLine(x, y, x+len, y);
        y += gridSize;
    }
}

/**
  set datas position
*/
void BoardWidget::setDataPosition(){
    for (int y=0; y<data.size(); ++y){
        for (int x=0; x<data[y].size(); ++x){
            Data& d = data[y][x];

            if (d.stone){
                delete d.stone;
                d.stone = createStoneItem(d.color, x, y);
            }
        }
    }
}

/**
  create buffer
*/
void BoardWidget::createBoardBuffer(){
    capturedWhite_ = 0;
    capturedBlack_ = 0;

    for (int i=0; i<buffer.size(); ++i)
        qFill(buffer[i], Go::eDame);

    Go::NodeList::iterator iter = currentNodeList_.begin();
    while (iter != currentNodeList_.end()){
        Go::NodePtr node(*iter);
        if (node->isStone() && !node->isPass() && node->x() < xsize() && node->y() < ysize()){
            buffer[node->y()][node->x()] = node->color();
            killStone(node->x(), node->y());
        }

        if (node == currentNode_)
            break;

        ++iter;
    }

    for(int y=0; y<ysize(); ++y){
        for(int x=0; x<xsize(); ++x){
            if (buffer[y][x] == data[y][x].color)
                continue;

            if (data[y][x].stone)
                delete data[y][x].stone;
            data[y][x].color = buffer[y][x];

            if (buffer[y][x] != Go::eDame){
                // create new stone item
                QGraphicsItem* stone = createStoneItem(buffer[y][x], x, y);
                if (stone == NULL)
                    return;

                data[y][x].stone = stone;
            }
            else
                data[y][x].stone = NULL;
        }
    }
}

/**
  kill around stone
*/
void BoardWidget::killStone(int x, int y){
    Go::Color color = buffer[y][x] == Go::eBlack ? Go::eWhite : Go::eBlack;
    killStone(x-1, y, color);
    killStone(x+1, y, color);
    killStone(x, y-1, color);
    killStone(x, y+1, color);
}

/**
  kill stone
*/
void BoardWidget::killStone(int x, int y, Go::Color color){
    if (x < 0 || x >= xsize() || y < 0 || y >= ysize())
        return;

    QVector< QVector<bool> > checked(ysize());
    for (int i=0; i<ysize(); ++i)
        checked[i].resize(xsize());

    if (isDeadStone(x, y, color, checked) == false)
        return;

    for (int y=0; y<checked.size(); ++y){
        for (int x=0; x<checked[y].size(); ++x){
            if (checked[y][x] == true && buffer[y][x] == color){
                buffer[y][x] = Go::eDame;
                if (color == Go::eWhite)
                    ++capturedWhite_;
                else if (color == Go::eBlack)
                    ++capturedBlack_;
            }
        }
    }
}

/**
  @retval true  this group is dead
  @retval false this group is alive
*/
bool BoardWidget::isDeadStone(int x, int y){
    if (buffer[y][x] == Go::eDame)
        return false;
    else if (isDeadStone(x, y, buffer[y][x]) == true)
        return true;

    return false;
}

/**
  @retval true  this group is dead
  @retval false this group is alive
*/
bool BoardWidget::isDeadStone(int x, int y, Go::Color color){
    if (x < 0 || x >= xsize() || y < 0 || y >= ysize())
        return false;

    QVector< QVector<bool> > checked(ysize());
    for (int i=0; i<ysize(); ++i)
        checked[i].resize(xsize());

    return isDeadStone(x, y, color, checked);
}

/**
  @retval true  this group is dead
  @retval false this group is alive
*/
bool BoardWidget::isDeadStone(int x, int y, Go::Color color, QVector< QVector<bool> >& checked){
    if (x < 0 || x >= xsize() || y < 0 || y >= ysize())
        return true;
    else if (checked[y][x] == true)
        return true;

    checked[y][x] = true;

    if (buffer[y][x] == Go::eDame)
        return false;
    else if (buffer[y][x] != color)
        return true;

    if (isDeadStone(x, y-1, color, checked) == false)
        return false;
    if (isDeadStone(x, y+1, color, checked) == false)
        return false;
    if (isDeadStone(x-1, y, color, checked) == false)
        return false;
    if (isDeadStone(x+1, y, color, checked) == false)
        return false;

    return true;
}

/**
  @retval true  (x,y) position's stone can kill arround group
  @retval false (x,y) position's stone can't kill arround group
*/
bool BoardWidget::isKillStone(int x, int y){
    Go::Color color;
    if (buffer[y][x] == Go::eBlack)
        color = Go::eWhite;
    else if (buffer[y][x] == Go::eWhite)
        color = Go::eBlack;
    else
        return false;

    if (isDeadStone(x-1, y, color) == true)
        return true;
    if (isDeadStone(x+1, y, color) == true)
        return true;
    if (isDeadStone(x, y-1, color) == true)
        return true;
    if (isDeadStone(x, y+1, color) == true)
        return true;

    return false;
}

/**
  create node list
*/
void BoardWidget::createNodeList(Go::NodePtr node){
    currentNodeList_.clear();

    Go::NodePtr n = node;
    while(n){
        currentNodeList_.push_front(n);
        n = n->parent();
    }

    n = node;
    while(n->children().empty() == false){
        n = n->child(0);
        currentNodeList_.push_back(n);
    }
}

/**
  create stone item
*/
QGraphicsItem* BoardWidget::createStoneItem(Go::Color color, int sgfX, int sgfY){
    // get stone position from sgf coordinate
    qreal x, y;
    if (sgfToViewCoordinate(sgfX, sgfY, x, y) == false)
        return NULL;

    // stone size
    qreal size = getGridSize() * 0.95;

    // create stone item
    QGraphicsEllipseItem* stone = new QGraphicsEllipseItem(x-size/2, y-size/2, size, size);
    stone->setPen(QPen(Qt::black));
    stone->setBrush(QBrush(color == Go::eBlack ? Qt::black : Qt::white));

    stone->setZValue(3);
    scene()->addItem(stone);

    return stone;
}

/**
  view coordinate to sgf coordinate
*/
bool BoardWidget::viewToSgfCoordinate(qreal viewX, qreal viewY, int& sgfX, int& sgfY) const{
    qreal size = getGridSize();

//    if ((rotate % 2) == 0){
        sgfX = (viewX - vLines[0]->line().x1() + size / 2.0) / size;
        sgfY = (viewY - hLines[0]->line().y1() + size / 2.0) / size;
//    }
//    else{
//        sgfX = (fabs(viewY - vLines[0]->line().y1()) + size / 2.0) / size;
//        sgfY = (fabs(viewX - hLines[0]->line().x1()) + size / 2.0) / size;
//    }

    return sgfX >= 0 && sgfX < xsize() && sgfY >= 0 && sgfY < ysize();
}

/**
  sgf coordinate to view coordinate
*/
bool BoardWidget::sgfToViewCoordinate(int sgfX, int sgfY, qreal& viewX, qreal& viewY) const{
//    if ((rotate % 2) == 0){
        viewX = vLines[sgfX]->line().x1();
        viewY = hLines[sgfY]->line().y1();
//    }
//    else{
//        sgfX = (fabs(viewY - vLines[0]->line().y1()) + size / 2.0) / size;
//        sgfY = (fabs(viewX - hLines[0]->line().x1()) + size / 2.0) / size;
//    }

    return true;
}

/**
  move stone
  create and put stone
*/
bool BoardWidget::alternateMove(int sgfX, int sgfY){
    // if stone already exist, can't put new stone.
    if (data[sgfY][sgfX].color != Go::eDame)
        return false;

    // can not suicide move
    buffer[sgfY][sgfX] = currentNode_->nextColor();
    if (isDeadStone(sgfX, sgfY) && isKillStone(sgfX, sgfY) == false){
        buffer[sgfY][sgfX] = Go::eDame;
        return false;
    }

    // create new node, and add to document
    Go::NodePtr node( Go::createStoneNode(currentNode_->nextColor(), sgfX, sgfY) );
    document()->undoStack()->push( new AddNodeCommand(document(), currentNode_, node, -1) );

    /* new node will be selected in on_document_nodeAdded
    // activate created node
    setNode(node);
    */

    return true;
}

/**
  move next node
*/
void BoardWidget::forward(int step){
    Go::NodeList::const_iterator iter = qFind(currentNodeList_.begin(), currentNodeList_.end(), currentNode_);
    if (iter == currentNodeList_.end())
        return;

    Go::NodeList::const_iterator pos = iter;
    for (int i=0; i<step; ++i){
        if (++iter == currentNodeList_.end())
            break;
        pos = iter;
    }
    setNode(*pos);
}

/**
  move previousn node
*/
void BoardWidget::back(int step){
    Go::NodeList::const_iterator iter = qFind(currentNodeList_.begin(), currentNodeList_.end(), currentNode_);
    if (iter == currentNodeList_.end())
        return;

    for (int i=0; i<step; ++i){
        if (iter == currentNodeList_.begin())
            break;
        --iter;
    }
    setNode(*iter);
}

/**
    node added
*/
void BoardWidget::on_document_nodeAdded(Go::NodePtr node){
    // createNodeList(currentNode_);
    setNode(node);
}

/**
    node deleted
*/
void BoardWidget::on_document_nodeDeleted(Go::NodePtr node){
    // return if deleted node isn't in the current node list
    Go::NodeList::const_iterator iter = qFind(currentNodeList_.begin(), currentNodeList_.end(), node);
    if (iter == currentNodeList_.end())
        return;

    // re-create node list
    createNodeList(node->parent());

    // if current node was deleted, select parent
    if (qFind(currentNodeList_.begin(), currentNodeList_.end(), currentNode_) == currentNodeList_.end())
        setNode(node->parent());
}
