#ifndef __sgf_h__
#define __sgf_h__

#include "godata.h"

namespace go{


class sgf : public fileBase{
public:
    enum eNodeType{ eUnknown, eRoot, eGameInformation, eBranch, eBlack, eWhite };

    class node;
    typedef QLinkedList<node*> nodeList;
    typedef QMap<QString, QStringList> propertyType;

    class node{
        public:
            node() : nodeType(eUnknown), x(-1), y(-1){}
            ~node();

            void clear(){
                childNodes.clear();
                property.clear();
            }

            nodeList& getChildNodes(){ return childNodes; }
            const nodeList& getChildNodes() const{ return childNodes; }

            void setNodeType(eNodeType type){ nodeType = type; }
            eNodeType getNodeType() const{ return nodeType; }

            bool setProperty(const QString& key, const QStringList& values);

            bool get(go::node& n) const;
            bool get(go::node& n, const QString& key, const QStringList& values) const;

            bool set(const go::node& n);
            bool set(const go::markList& markList);
            bool set(const go::stoneList& markList);

            QString toString() const;

        private:
            void setPosition(eNodeType type, const QString& pos);
            bool getPosition(const QString& pos, int& x, int& y, QString* str=NULL) const;
            void addMark(go::markList& markList, const QStringList& values, const char* str=NULL) const;
            void addMark(go::markList& markList, const QStringList& values, mark::eType type) const;
            void addStone(go::stoneList& stoneList, const QString& key, const QStringList& values) const;

            nodeList childNodes;
            eNodeType nodeType;
            propertyType property;
            int x, y;
    };


    sgf(){
        root.setNodeType(eRoot);
    }

    node& getRoot(){ return root; }

    virtual bool readStream(QString::iterator& first, QString::iterator last);
    virtual bool saveStream(QTextStream& stream);

    virtual bool get(go::data& data) const;
    virtual bool set(const go::data& data);

    static QString pointToString(int x, int y, const QString* s=NULL);
    static QString pointToString(const go::point& p, const QString* s=NULL);

protected:
    bool readBranch(QString::iterator& first, QString::iterator last, node& n);
    bool readNode(QString::iterator& first, QString::iterator last, node& n);
    bool readNode2(QString::iterator& first, QString::iterator last, node& n);
    bool readNodeKey(QString::iterator& first, QString::iterator last, QString& key);
    bool readNodeValues(QString::iterator& first, QString::iterator last, QStringList& values);
    bool readNodeValue(QString::iterator& first, QString::iterator last, QString& value);

    bool writeNode(QTextStream& stream, const node& n);

    go::node* get(const node& sgfNode, go::node* outNode) const;
    bool set(sgf::node* node, const go::node* node2);

    node root;
};


}



#endif
