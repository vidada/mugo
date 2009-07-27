#include <QDebug>
#include "appdef.h"
#include "godata.h"

namespace go{


node::node(data* data_) : goData(data_), parent(NULL){
}

node::node(node* parent_) : goData(parent_->goData), parent(parent_){
}

void node::clear(){
    parent = NULL;

    nodeList::iterator iter = childNodes.begin();
    while (iter != childNodes.end()){
            delete *iter;
            ++iter;
    }
    childNodes.clear();
}

QString node::toString() const{
    QString str = nodeName();

    if (!stones.empty())
        str.append(" Add");

    if (!crosses.empty() || !triangles.empty() || !squares.empty() || !circles.empty() || !characters.empty())
        str.append(" Mark");

    if (!blackTerritories.empty())
        str.append(" BlackTerritories");

    if (!whiteTerritories.empty())
        str.append(" WhiteTerritories");

    if (!comment.isEmpty())
        str.append(" Comment");

    if (!str.isEmpty() && str[0] == ' ')
        str.remove(0, 1);

    return str;
}

void informationNode::initialize(){
    size = 19;
    komi = 6.5;
    handicap = 0;
}

QString stoneNode::nodeName() const{
    return "";
}


void data::clear(){
    root.clear();
    root = informationNode(this);
}


bool fileBase::read(const QString& fname, QTextCodec* codec){
    QFile f(fname);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QByteArray a = f.read( f.size() );
    QString s = codec->toUnicode(a);
    QString::iterator iter = s.begin();
    return readStream(iter, s.end());
}

bool fileBase::save(const QString& fname, QTextCodec* codec){
    QFile f(fname);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    QString str;
    QTextStream stream(&str, QIODevice::WriteOnly);
    bool ret = saveStream(stream);
    stream.flush();

    QByteArray a = codec->fromUnicode(str);
    f.write(a);

    return ret;
}



sgf::node::~node(){
    nodeList::iterator iter = childNodes.begin();
    while (iter != childNodes.end()){
            delete *iter;
            ++iter;
    }
}

QString sgf::node::toString() const{
    static const QString keys[] = {
        // game information
        "GM", "FF", "CA", "AP", "SZ", "KM", "HA", "RU",
        "PW", "WR", "WT", "PB", "BR", "BT", "RE", "TM", "OT",
        "DT", "GN", "RO", "PC", "EV",
        "GC", "ON", "AN", "CP", "SO", "US",

        // stone
        "B", "W",

        // marker
        "AB", "AW", "AE", "MA", "CR", "SQ", "TR", "LB", "TB", "TW",

        "C"
    };
    static const int N = sizeof(keys) / sizeof(keys[0]);

    QString str;
    str.reserve(50);
    str.push_back(';');

    for (int i=0; i<N; ++i){
        propertyType::const_iterator iter = property.find(keys[i]);
        if (iter == property.end())
            continue;

        const propertyType::key_type& key = iter.key();
        const propertyType::mapped_type& values = iter.value();
        str.push_back(key);

        propertyType::mapped_type::const_iterator iter2 = values.begin();
        while (iter2 != values.end()){
            str.push_back('[');
            QString tmp(*iter2);
            tmp.replace(']', "\\]");
            str.push_back(tmp);
            str.push_back(']');
            ++iter2;
        }
    }

    return str;
}

bool sgf::node::setProperty(const QString& key, const QStringList& values){
    if (key == "B" || key == "W"){
        if (values.size() != 1)
            return false;
        setPosition(key == "B" ? eBlack : eWhite, values[0]);
    }

    property[key] = values;

    return true;
}

void sgf::node::setPosition(sgf::eNodeType type, const QString& pos){
    nodeType = type;
    getPosition(pos, x, y);
}

bool sgf::node::getPosition(const QString& pos, int& x, int& y, QString* str) const{
    if (pos.size() < 2)
        return false;
    x = pos[0].unicode() - 'a';
    y = pos[1].unicode() - 'a';

    if (str && pos.size() > 3 && pos[2] == ':')
        *str = pos.mid(3);

    return true;
}

void sgf::node::addMark(go::markList& markList, const QStringList& values, const char* str) const{
    QStringList::const_iterator iter = values.begin();
    while (iter != values.end()){
        int x, y;
        QString str2;
        if (getPosition(*iter, x, y, &str2))
            markList.push_back( go::mark(x, y, str2.isEmpty() ? str : str2) );
        ++iter;
    }
}

void sgf::node::addMark(go::markList& markList, const QStringList& values, mark::eType type) const{
    QStringList::const_iterator iter = values.begin();
    while (iter != values.end()){
        int x, y;
        if (getPosition(*iter, x, y))
            markList.push_back( go::mark(x, y, type) );
        ++iter;
    }
}

void sgf::node::addStone(go::stoneList& stoneList, const QString& key, const QStringList& values) const{
    go::stone::color c = key == "AB" ? go::stone::black : key == "AW" ? go::stone::white : go::stone::empty;

    QStringList::const_iterator iter = values.begin();
    while (iter != values.end()){
        int x, y;
        if (getPosition(*iter, x, y))
            stoneList.push_back( go::stone(x, y, c) );
        ++iter;
    }
}

bool sgf::node::get(go::node& n) const{
    if (nodeType == eBlack || nodeType == eWhite){
        n.setX(x);
        n.setY(y);
    }

    propertyType::const_iterator iter = property.begin();
    while (iter != property.end()){
        get(n, iter.key(), iter.value());
        ++iter;
    }

    return true;
}

bool sgf::node::set(const go::node& n){
    const go::informationNode* infoNode = dynamic_cast<const go::informationNode*>(&n);
    const go::stoneNode* stoneNode = dynamic_cast<const go::stoneNode*>(&n);
    if (infoNode){
        property["GM"].push_back("1");
        property["FF"].push_back("4");
        property["AP"].push_back(APP_NAME ":" VERSION);
        property["SZ"].push_back( QString("%1").arg(infoNode->size) );
        property["KM"].push_back( QString("%1").arg(infoNode->komi) );
        property["HA"].push_back( QString("%1").arg(infoNode->handicap) );
        property["RU"].push_back( infoNode->rule );

        property["PW"].push_back( infoNode->whitePlayer );
        property["WR"].push_back( infoNode->whiteRank );
        property["WT"].push_back( infoNode->whiteTeam );
        property["PB"].push_back( infoNode->blackPlayer );
        property["BR"].push_back( infoNode->blackRank );
        property["BT"].push_back( infoNode->blackTeam );
        property["RE"].push_back( infoNode->result );
        property["TM"].push_back( infoNode->time );
        property["OT"].push_back( infoNode->overTime );

        property["DT"].push_back( infoNode->date );
        property["GN"].push_back( infoNode->gameName );
        property["RO"].push_back( infoNode->round );
        property["PC"].push_back( infoNode->place );
        property["EV"].push_back( infoNode->event );

        property["GC"].push_back( infoNode->gameComment );
        property["AN"].push_back( infoNode->annotation );
        property["CP"].push_back( infoNode->copyright );
        property["ON"].push_back( infoNode->opening );
        property["SO"].push_back( infoNode->source );
        property["US"].push_back( infoNode->user );
    }
    else if (stoneNode){
        x = n.position.x;
        y = n.position.y;
        QString str;
        if (x != -1 && y != -1)
            str.sprintf("%c%c", 'a' + x, 'a' + y);
        propertyType::key_type key = stoneNode->isBlack() ? "B" : "W";
        property[key].push_back(str);
    }

    if (!n.comment.isEmpty())
        property["C"].push_back(n.comment);

    // marker
    set(n.crosses);
    set(n.circles);
    set(n.triangles);
    set(n.squares);
    set(n.characters);
    set(n.blackTerritories);
    set(n.whiteTerritories);
    set(n.stones);

    return false;
}

bool sgf::node::set(const go::markList& markList){
    markList::const_iterator iter = markList.begin();
    while (iter != markList.end()){
        switch (iter->t){
            case go::mark::eCross:
                property["MA"].push_back( pointToString(iter->p) );
                break;
            case go::mark::eCircle:
                property["CR"].push_back( pointToString(iter->p) );
                break;
            case go::mark::eSquare:
                property["SQ"].push_back( pointToString(iter->p) );
                break;
            case go::mark::eTriangle:
                property["TR"].push_back( pointToString(iter->p) );
                break;
            case go::mark::eCharacter:
                property["LB"].push_back( pointToString(iter->p, &iter->s) );
                break;
            case go::mark::eBlackTerritory:
                property["TB"].push_back( pointToString(iter->p) );
                break;
            case go::mark::eWhiteTerritory:
                property["TW"].push_back( pointToString(iter->p) );
                break;
        };
        ++iter;
    }

    return true;
}

bool sgf::node::set(const go::stoneList& stoneList){
    go::stoneList::const_iterator iter = stoneList.begin();
    while (iter != stoneList.end()){
        if (iter->isBlack())
            property["AB"].push_back( pointToString(iter->p) );
        else if (iter->isWhite())
            property["AW"].push_back( pointToString(iter->p) );
        else if (iter->isEmpty())
            property["AE"].push_back( pointToString(iter->p) );
        ++iter;
    }

    return true;
}

bool sgf::node::get(go::node& n, const QString& key, const QStringList& values) const{
    go::informationNode* infoNode = dynamic_cast<go::informationNode*>(&n);

    // game information
    if (infoNode && key == "PW" && !values.empty())
        infoNode->whitePlayer = values[0];
    else if (infoNode && key == "WR" && !values.empty())
        infoNode->whiteRank = values[0];
    else if (infoNode && key == "WT" && !values.empty())
        infoNode->whiteTeam = values[0];
    if (infoNode && key == "PB" && !values.empty())
        infoNode->blackPlayer = values[0];
    else if (infoNode && key == "BR" && !values.empty())
        infoNode->blackRank = values[0];
    else if (infoNode && key == "BT" && !values.empty())
        infoNode->blackTeam = values[0];
    else if (infoNode && key == "RE" && !values.empty())
        infoNode->result = values[0];
    else if (infoNode && key == "SZ" && !values.empty())
        infoNode->size = values[0].toInt();
    else if (infoNode && key == "KM" && !values.empty())
        infoNode->komi = values[0].toDouble();
    else if (infoNode && key == "HA" && !values.empty())
        infoNode->handicap = values[0].toInt();
    else if (infoNode && key == "TM" && !values.empty())
        infoNode->time = values[0];
    else if (infoNode && key == "OT" && !values.empty())
        infoNode->overTime = values[0];
    else if (infoNode && key == "DT" && !values.empty())
        infoNode->date = values[0];
    else if (infoNode && key == "GN" && !values.empty())
        infoNode->gameName = values[0];
    else if (infoNode && key == "RO" && !values.empty())
        infoNode->round = values[0];
    else if (infoNode && key == "PC" && !values.empty())
        infoNode->place = values[0];
    else if (infoNode && key == "EV" && !values.empty())
        infoNode->event = values[0];
    else if (infoNode && key == "GC" && !values.empty())
        infoNode->gameComment = values[0];
    else if (infoNode && key == "AN" && !values.empty())
        infoNode->annotation = values[0];
    else if (infoNode && key == "CP" && !values.empty())
        infoNode->copyright = values[0];
    else if (infoNode && key == "ON" && !values.empty())
        infoNode->opening = values[0];
    else if (infoNode && key == "RU" && !values.empty())
        infoNode->rule = values[0];
    else if (infoNode && key == "SO" && !values.empty())
        infoNode->source = values[0];
    else if (infoNode && key == "US" && !values.empty())
        infoNode->user = values[0];

    // comment
    else if (key == "C" && !values.empty())
        n.comment = values[0];

    // mark
    else if (key == "MA" || key == "M")
        addMark(n.crosses, values, mark::eCross);
    else if (key == "TR")
        addMark(n.triangles, values, mark::eTriangle);
    else if (key == "CR")
        addMark(n.circles, values, mark::eCircle);
    else if (key == "SQ")
        addMark(n.squares, values, mark::eSquare);
    else if (key == "LB")
        addMark(n.characters, values);
    else if (key == "TB")
        addMark(n.blackTerritories, values, mark::eBlackTerritory);
    else if (key == "TW")
        addMark(n.whiteTerritories, values, mark::eWhiteTerritory);
    else if (key == "AB" || key == "AW" || key == "AE")
        addStone(n.stones, key, values);

    return true;
}

bool sgf::readStream(QString::iterator& first, QString::iterator last){
    bool ret = false;

    while (first != last){
        QChar c = *first++;
        if (c == '('){
            ret = readBranch(first, last, root);
            root.setNodeType(eRoot);
            break;
        }
    }

    if (!root.getChildNodes().empty())
        root.getChildNodes().front()->setNodeType(eGameInformation);

    return ret;
}

bool sgf::saveStream(QTextStream& stream){
    return writeNode(stream, root);
}

bool sgf::readBranch(QString::iterator& first, QString::iterator last, node& n){
    n.setNodeType(sgf::eBranch);
    while (first != last){
        QChar c = *first++;
        if (c == ')')
            return true;
        else if (c != ';' && c != '(')
            continue;

        if (readNode(--first, last, n) == false)
            return false;
    }

    return false;
}

bool sgf::readNode(QString::iterator& first, QString::iterator last, node& n){
    while (first != last){
        QChar c = *first++;
        if (c == ';'){
            node* newNode = new node;
            n.getChildNodes().push_back(newNode);

            if (readNode2(first, last, *newNode) == false)
                return false;
            else
                return true;
        }
        else if (c == '('){
            node* newNode = new node;
            n.getChildNodes().push_back(newNode);
            return readBranch(first, last, *newNode);
        }
    }

    return true;
}

bool sgf::readNode2(QString::iterator& first, QString::iterator last, node& n){
    while (first != last){
        QChar c = *first;
        if (c == '\r' || c == '\n'){
            ++first;
            continue;
        }
        else if (c == ';' || c == '(' || c == ')'){
            break;
        }

        QString key;
        if (readNodeKey(first, last, key) == false)
            return false;

        QStringList values;
        if (readNodeValues(first, last, values) == false)
            return false;

        n.setProperty(key, values);
    }

    return true;
}

bool sgf::readNodeKey(QString::iterator& first, QString::iterator last, QString& key){
    while (first != last){
        if (*first == '[')
            return true;
        else
            key.push_back(*first++);
    }

    return false;
}

bool sgf::readNodeValues(QString::iterator& first, QString::iterator last, QStringList& values){
    while (first != last){
        if (*first == '['){
            QString v;
            if (readNodeValue(++first, last, v))
                values.push_back(v);
        }
        else
            return true;
    }

    return false;
}

bool sgf::readNodeValue(QString::iterator& first, QString::iterator last, QString& value){
    while (first != last){
        QChar c = *first++;

        if (c == ']')
            return true;

        if (c == '\\')
            c = *first++;
        value.push_back(c);
    }

    return false;
}

bool sgf::writeNode(QTextStream& stream, const node& n){
    if (n.getNodeType() == eBranch || n.getNodeType() == eRoot){
        stream << '(';
        nodeList::const_iterator iter = n.getChildNodes().begin();
        while (iter != n.getChildNodes().end()){
            writeNode(stream, **iter);
            ++iter;
        }
        stream << ")\n";
    }
    else{
        stream << n.toString() << '\n';
        nodeList::const_iterator iter = n.getChildNodes().begin();
        while (iter != n.getChildNodes().end()){
            writeNode(stream, **iter);
            ++iter;
        }
    }

    return true;
}

bool sgf::get(go::data& data) const{
    data.clear();
    get(root, &data.root);

    return true;
}

go::node* sgf::get(const node& sgfNode, go::node* outNode) const{
    // ノードを処理
    go::node* newNode = NULL;
    switch(sgfNode.getNodeType()){
        case eBlack:
            newNode = new go::blackNode(outNode);
            break;

        case eWhite:
            newNode = new go::whiteNode(outNode);
            break;

        case eBranch:
//            newNode = new go::branchNode(outNode.getChildNodes().empty() ? &outNode : outNode.getChildNodes().back());
            break;

        case eGameInformation:
//            newNode = new go::informationNode(&outNode);
            sgfNode.get(*outNode);
            break;

        case eRoot:
            break;

        default:
            newNode = new go::node(outNode);
            break;
    }

    if (newNode){
        outNode->childNodes.push_back(newNode);
        sgfNode.get(*newNode);
    }
    else
        newNode = outNode;

    // 子要素を処理
    go::node* childNode = newNode;
    nodeList::const_iterator iter = sgfNode.getChildNodes().begin();
    while (iter != sgfNode.getChildNodes().end()){
        childNode = get(**iter, childNode);
        ++iter;
    }

    return newNode;
}

bool sgf::set(const go::data& data){
    root.clear();
    root.setNodeType(eRoot);

    node* gameInfo = new node;
    gameInfo->set(data.root);
    root.getChildNodes().push_back(gameInfo);

    return set(gameInfo, &data.root);
}

bool sgf::set(node* sgfNode, const go::node* goNode){
    if (goNode->childNodes.size() > 1){
        go::nodeList::const_iterator iter = goNode->childNodes.begin();
        while (iter != goNode->childNodes.end()){
            node* branchNode = new node;
            branchNode->setNodeType(eBranch);
            sgfNode->getChildNodes().push_back(branchNode);

            node* newNode = new node;
            newNode->set( **iter );
            branchNode->getChildNodes().push_back(newNode);
            set(newNode, *iter);
            ++iter;
        }
    }
    else if (goNode->childNodes.size() == 1){
        node* newNode = new node;
        newNode->set( *goNode->childNodes.front() );
        sgfNode->getChildNodes().push_back(newNode);
        set(sgfNode, goNode->childNodes.front());
    }

    return true;
}

QString sgf::pointToString(int x, int y, const QString* s){
    char buf[10];
    if (s)
        sprintf(buf, "%c%c:%s", 'a' + x, 'a' + y, (const char*)s->toAscii());
    else
        sprintf(buf, "%c%c", 'a' + x, 'a' + y);
    return QString(buf);
}

QString sgf::pointToString(const go::point& p, const QString* s){
    return pointToString(p.x, p.y, s);
}


};
