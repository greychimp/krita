#ifndef KISBEZIERMESH_H
#define KISBEZIERMESH_H

#include <kritaglobal_export.h>

#include <QDebug>

#include <KisBezierUtils.h>
#include <KisBezierPatch.h>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/operators.hpp>

#include <functional>

#include "kis_debug.h"

class QDomElement;

namespace KisBezierMeshDetails {

struct BaseMeshNode : public boost::equality_comparable<BaseMeshNode> {
    BaseMeshNode() {}
    BaseMeshNode(const QPointF &_node)
        : leftControl(_node),
          topControl(_node),
          node(_node),
          rightControl(_node),
          bottomControl(_node)
    {
    }

    bool operator==(const BaseMeshNode &rhs) const {
        return leftControl == rhs.leftControl &&
                topControl == rhs.topControl &&
                node == rhs.node &&
                rightControl == rhs.rightControl &&
                bottomControl == rhs.bottomControl;
    }

    void setLeftControlRelative(const QPointF &value) {
        leftControl = value + node;
    }

    QPointF leftControlRelative() const {
        return leftControl - node;
    }

    void setRightControlRelative(const QPointF &value) {
        rightControl = value + node;
    }

    QPointF rightControlRelative() const {
        return rightControl - node;
    }

    void setTopControlRelative(const QPointF &value) {
        topControl = value + node;
    }

    QPointF topControlRelative() const {
        return topControl - node;
    }

    void setBottomControlRelative(const QPointF &value) {
        bottomControl = value + node;
    }

    QPointF bottomControlRelative() const {
        return bottomControl - node;
    }

    void translate(const QPointF &offset) {
        leftControl += offset;
        topControl += offset;
        node += offset;
        rightControl += offset;
        bottomControl += offset;
    }

    void transform(const QTransform &t) {
        leftControl = t.map(leftControl);
        topControl = t.map(topControl);
        node = t.map(node);
        rightControl = t.map(rightControl);
        bottomControl = t.map(bottomControl);
    }

    QPointF leftControl;
    QPointF topControl;
    QPointF node;
    QPointF rightControl;
    QPointF bottomControl;
};

inline void lerpNodeData(const BaseMeshNode &left, const BaseMeshNode &right, qreal t, BaseMeshNode &dst)
{
    Q_UNUSED(left);
    Q_UNUSED(right);
    Q_UNUSED(t);
    Q_UNUSED(dst);
}

inline void assignPatchData(KisBezierPatch *patch,
                            const QRectF &srcRect,
                            const BaseMeshNode &tl,
                            const BaseMeshNode &tr,
                            const BaseMeshNode &bl,
                            const BaseMeshNode &br)
{
    patch->originalRect = srcRect;
    Q_UNUSED(tl);
    Q_UNUSED(tr);
    Q_UNUSED(bl);
    Q_UNUSED(br);
}

template<typename NodeArg = BaseMeshNode,
         typename PatchArg = KisBezierPatch>
class Mesh : public boost::equality_comparable<Mesh<NodeArg, PatchArg>>
{
public:
    using Node = NodeArg;
    using Patch = PatchArg;
    using PatchIndex = QPoint;
    using NodeIndex = QPoint;
    using SegmentIndex = std::pair<NodeIndex, int>;

    struct ControlPointIndex : public boost::equality_comparable<ControlPointIndex>
    {
        enum ControlType {
            LeftControl = 0,
            TopControl,
            RightControl,
            BottomControl,
            Node
        };

        ControlPointIndex()  = default;
        ControlPointIndex(const ControlPointIndex &rhs) = default;
        ControlPointIndex(NodeIndex _nodeIndex, ControlType _controlType)
            : nodeIndex(_nodeIndex),
              controlType(_controlType)
        {
        }

        NodeIndex nodeIndex;
        ControlType controlType;

        inline bool isNode() const {
            return controlType == Node;
        }

        inline bool isControlPoint() const {
            return controlType != Node;
        }

        static QPointF& controlPoint(Mesh::Node &node, ControlType controlType) {
            return
                controlType == LeftControl ? node.leftControl :
                controlType == RightControl ? node.rightControl :
                controlType == TopControl ? node.topControl :
                controlType == BottomControl ? node.bottomControl :
                node.node;
        }

        QPointF& controlPoint(Mesh::Node &node) {
            return controlPoint(node, controlType);
        }

        friend bool operator==(const ControlPointIndex &lhs, const ControlPointIndex &rhs) {
            return lhs.nodeIndex == rhs.nodeIndex && lhs.controlType == rhs.controlType;
        }

        friend QDebug operator<<(QDebug dbg, const Mesh::ControlPointIndex &index) {
            dbg.nospace() << "ControlPointIndex ("
                          << index.nodeIndex.x() << ", " << index.nodeIndex.x() << ", ";

            switch (index.controlType) {
            case Mesh::ControlType::Node:
                dbg.nospace() << "Node";
                break;
            case Mesh::ControlType::LeftControl:
                dbg.nospace() << "LeftControl";
                break;
            case Mesh::ControlType::RightControl:
                dbg.nospace() << "RightControl";
                break;
            case Mesh::ControlType::TopControl:
                dbg.nospace() << "TopControl";
                break;
            case Mesh::ControlType::BottomControl:
                dbg.nospace() << "BottomControl";
                break;
            }

            dbg.nospace() << ")";
            return dbg.space();
        }

    };

    using ControlType = typename ControlPointIndex::ControlType;


public:
    Mesh()
        : Mesh(QRectF(0.0, 0.0, 777.0, 666.0))
    {
    }

    Mesh(const QRectF &mapRect, const QSize &size = QSize(2,2))
        : m_size(size),
          m_originalRect(mapRect)
    {
        const qreal xControlOffset = 0.2 * (mapRect.width() / size.width());
        const qreal yControlOffset = 0.2 * (mapRect.height() / size.height());

        for (int row = 0; row < m_size.height(); row++) {
            const qreal yPos = qreal(row) / (size.height() - 1) * mapRect.height() + mapRect.y();

            for (int col = 0; col < m_size.width(); col++) {
                const qreal xPos = qreal(col) / (size.width() - 1) * mapRect.width() + mapRect.x();

                Node node(QPointF(xPos, yPos));
                node.setLeftControlRelative(QPointF(-xControlOffset, 0));
                node.setRightControlRelative(QPointF(xControlOffset, 0));
                node.setTopControlRelative(QPointF(0, -yControlOffset));
                node.setBottomControlRelative(QPointF(0, yControlOffset));

                m_nodes.push_back(node);
            }
        }

        for (int col = 0; col < m_size.width(); col++) {
            m_columns.push_back(qreal(col) / (size.width() - 1));
        }

        for (int row = 0; row < m_size.height(); row++) {
            m_rows.push_back(qreal(row) / (size.height() - 1));
        }
    }

    bool operator==(const Mesh &rhs) const {
        return m_size == rhs.m_size &&
                m_rows == rhs.m_rows &&
                m_columns == rhs.m_columns &&
                m_originalRect == rhs.m_originalRect &&
                m_nodes == rhs.m_nodes;
    }

    void splitCurveHorizontally(Node &left, Node &right, qreal t, Node &newNode) {
        using KisBezierUtils::deCasteljau;
        using KisAlgebra2D::lerp;

        QPointF p1, p2, p3, q1, q2;

        deCasteljau(left.node, left.rightControl, right.leftControl, right.node, t,
                    &p1, &p2, &p3, &q1, &q2);

        left.rightControl = p1;
        newNode.leftControl = p2;
        newNode.node = p3;
        newNode.rightControl = q1;
        right.leftControl = q2;

        newNode.topControl = newNode.node + lerp(left.topControl - left.node, right.topControl - right.node, t);
        newNode.bottomControl = newNode.node + lerp(left.bottomControl - left.node, right.bottomControl - right.node, t);

        lerpNodeData(left, right, t, newNode);
    }

    Node& node(int col, int row) {
        return m_nodes[row * m_size.width() + col];
    }

    const Node& node(int col, int row) const {
        return m_nodes[row * m_size.width() + col];
    }

    Node& node(const NodeIndex &index) {
        return node(index.x(), index.y());
    }

    const Node& node(const NodeIndex &index) const {
        return node(index.x(), index.y());
    }

    void splitCurveVertically(Node &top, Node &bottom, qreal t, Node &newNode) {
        using KisBezierUtils::deCasteljau;
        using KisAlgebra2D::lerp;

        QPointF p1, p2, p3, q1, q2;

        deCasteljau(top.node, top.bottomControl, bottom.topControl, bottom.node, t,
                    &p1, &p2, &p3, &q1, &q2);

        top.bottomControl = p1;
        newNode.topControl = p2;
        newNode.node = p3;
        newNode.bottomControl = q1;
        bottom.topControl = q2;

        newNode.leftControl = newNode.node + lerp(top.leftControl - top.node, bottom.leftControl - bottom.node, t);
        newNode.rightControl = newNode.node + lerp(top.rightControl - top.node, bottom.rightControl - bottom.node, t);

        lerpNodeData(top, bottom, t, newNode);
    }

    void subdivideRow(qreal t) {
        if (qFuzzyCompare(t, 0.0) || qFuzzyCompare(t, 1.0)) return;

        KIS_SAFE_ASSERT_RECOVER_RETURN(t > 0.0 && t < 1.0);

        const auto it = prev(upper_bound(m_rows.begin(), m_rows.end(), t));
        const int topRow = distance(m_rows.begin(), it);
        const qreal relT = (t - *it) / (*next(it) - *it);

        subdivideRow(topRow, relT);
    }

    void subdivideRow(int topRow, qreal relT) {
        const auto it = m_rows.begin() + topRow;
        const int bottomRow = topRow + 1;
        const qreal absT = KisAlgebra2D::lerp(*it, *next(it), relT);

        std::vector<Node> newRow;
        newRow.resize(m_size.width());
        for (int col = 0; col < m_size.width(); col++) {
            const qreal paramForCurve =
                KisBezierUtils::curveParamByProportion(node(col, topRow).node,
                                                       node(col, topRow).bottomControl,
                                                       node(col, bottomRow).topControl,
                                                       node(col, bottomRow).node,
                                                       relT,
                                                       0.01);

            splitCurveVertically(node(col, topRow), node(col, bottomRow), paramForCurve, newRow[col]);
        }

        m_nodes.insert(m_nodes.begin() + bottomRow * m_size.width(),
                       newRow.begin(), newRow.end());

        m_size.rheight()++;
        m_rows.insert(next(it), absT);
    }

    void subdivideColumn(qreal t) {
        if (qFuzzyCompare(t, 0.0) || qFuzzyCompare(t, 1.0)) return;

        KIS_SAFE_ASSERT_RECOVER_RETURN(t > 0.0 && t < 1.0);

        const auto it = prev(upper_bound(m_columns.begin(), m_columns.end(), t));
        const int leftColumn = distance(m_columns.begin(), it);

        const qreal relT = (t - *it) / (*next(it) - *it);

        subdivideColumn(leftColumn, relT);
    }

    void subdivideColumn(int leftColumn, qreal relT) {
        const auto it = m_columns.begin() + leftColumn;
        const int rightColumn = leftColumn + 1;
        const qreal absT = KisAlgebra2D::lerp(*it, *next(it), relT);

        std::vector<Node> newColumn;
        newColumn.resize(m_size.height());
        for (int row = 0; row < m_size.height(); row++) {
            const qreal paramForCurve =
                KisBezierUtils::curveParamByProportion(node(leftColumn, row).node,
                                                       node(leftColumn, row).rightControl,
                                                       node(rightColumn, row).leftControl,
                                                       node(rightColumn, row).node,
                                                       relT,
                                                       0.01);

            splitCurveHorizontally(node(leftColumn, row), node(rightColumn, row), paramForCurve, newColumn[row]);
        }

        auto dstIt = m_nodes.begin() + rightColumn;
        for (auto columnIt = newColumn.begin(); columnIt != newColumn.end(); ++columnIt) {
            dstIt = m_nodes.insert(dstIt, *columnIt);
            dstIt += m_size.width() + 1;
        }

        m_size.rwidth()++;
        m_columns.insert(next(it), absT);
    }

    void unlinkNodeHorizontally(Mesh::Node &left, const Mesh::Node &node, Mesh::Node &right)
    {
        std::tie(left.rightControl, right.leftControl) =
            KisBezierUtils::removeBezierNode(left.node, left.rightControl,
                                             node.leftControl, node.node, node.rightControl,
                                             right.leftControl, right.node);
    }

    void unlinkNodeVertically(Mesh::Node &top, const Mesh::Node &node, Mesh::Node &bottom)
    {
        std::tie(top.bottomControl, bottom.topControl) =
            KisBezierUtils::removeBezierNode(top.node, top.bottomControl,
                                             node.topControl, node.node, node.bottomControl,
                                             bottom.topControl, bottom.node);
    }


    void removeColumn(int column) {
        const bool hasNeighbours = column > 0 || column < m_size.width() - 1;

        if (hasNeighbours) {
            for (int row = 0; row < m_size.height(); row++) {
                unlinkNodeHorizontally(node(column - 1, row), node(column, row), node(column + 1, row));
            }
        }

        auto it = m_nodes.begin() + column;
        for (int row = 0; row < m_size.height(); row++) {
            it = m_nodes.erase(it);
            it += m_size.width() - 1;
        }

        m_size.rwidth()--;
        m_columns.erase(m_columns.begin() + column);
    }

    void removeRow(int row) {
        const bool hasNeighbours = row > 0 || row < m_size.height() - 1;

        if (hasNeighbours) {
            for (int column = 0; column < m_size.width(); column++) {
                unlinkNodeVertically(node(column, row - 1), node(column, row), node(column, row + 1));
            }
        }

        auto it = m_nodes.begin() + row * m_size.width();
        m_nodes.erase(it, it + m_size.width());

        m_size.rheight()--;
        m_rows.erase(m_rows.begin() + row);
    }

    void removeColumnOrRow(NodeIndex index, bool row) {
        if (row) {
            removeRow(index.y());
        } else {
            removeColumn(index.x());
        }
    }

    void subdivideSegment(SegmentIndex index, qreal proportion) {
        auto it = find(index);
        KIS_SAFE_ASSERT_RECOVER_RETURN(it != endSegments());

        if (it.isHorizontal()) {
            subdivideColumn(it.firstNodeIndex().x(), proportion);
        } else {
            subdivideRow(it.firstNodeIndex().y(), proportion);
        }
    }

    Patch makePatch(const PatchIndex &index) const {
        return makePatch(index.x(), index.y());
    }

    Patch makePatch(int col, int row) const
    {
        const Node &tl = node(col, row);
        const Node &tr = node(col + 1, row);
        const Node &bl = node(col, row + 1);
        const Node &br = node(col + 1, row + 1);

        Patch patch;

        patch.points[Patch::TL] = tl.node;
        patch.points[Patch::TL_HC] = tl.rightControl;
        patch.points[Patch::TL_VC] = tl.bottomControl;

        patch.points[Patch::TR] = tr.node;
        patch.points[Patch::TR_HC] = tr.leftControl;
        patch.points[Patch::TR_VC] = tr.bottomControl;

        patch.points[Patch::BL] = bl.node;
        patch.points[Patch::BL_HC] = bl.rightControl;
        patch.points[Patch::BL_VC] = bl.topControl;

        patch.points[Patch::BR] = br.node;
        patch.points[Patch::BR_HC] = br.leftControl;
        patch.points[Patch::BR_VC] = br.topControl;

        const QRectF relRect(m_columns[col],
                             m_rows[row],
                             m_columns[col + 1] - m_columns[col],
                             m_rows[row + 1] - m_rows[row]);

        assignPatchData(&patch, KisAlgebra2D::relativeToAbsolute(relRect, m_originalRect),
                        tl, tr, bl, br);

        return patch;
    }


    class segment_iterator;

    class patch_iterator :
        public boost::iterator_facade <patch_iterator,
                                       Patch,
                                       boost::random_access_traversal_tag,
                                       Patch>
    {
    public:
        patch_iterator()
            : m_mesh(0),
              m_col(0),
              m_row(0) {}

        patch_iterator(const Mesh* mesh, int col, int row)
            : m_mesh(mesh),
              m_col(col),
              m_row(row)
        {
        }

        Mesh::PatchIndex patchIndex() const {
            return {m_col, m_row};
        }

        segment_iterator segmentP() const;
        segment_iterator segmentQ() const;
        segment_iterator segmentR() const;
        segment_iterator segmentS() const;

        bool isValid() const {
            return
                m_col >= 0 &&
                m_col < m_mesh->size().width() - 1 &&
                m_row >= 0 &&
                m_row < m_mesh->size().height() - 1;
        }

    private:
        friend class boost::iterator_core_access;

        void increment() {
            m_col++;
            if (m_col >= m_mesh->m_size.width() - 1) {
                m_col = 0;
                m_row++;
            }
        }

        void decrement() {
            m_col--;
            if (m_col < 0) {
                m_col = m_mesh->m_size.width() - 2;
                m_row--;
            }
        }

        void advance(int n) {
            const int index = m_row * (m_mesh->m_size.width() - 1) + m_col + n;

            m_row = index / (m_mesh->m_size.width() - 1);
            m_col = index % (m_mesh->m_size.width() - 1);

            KIS_SAFE_ASSERT_RECOVER_NOOP(m_row < m_mesh->m_size.height() - 1);
        }

        int distance_to(const patch_iterator &z) const {
            const int index = m_row * (m_mesh->m_size.width() - 1) + m_col;
            const int otherIndex = z.m_row * (m_mesh->m_size.width() - 1) + z.m_col;

            return otherIndex - index;
        }

        bool equal(patch_iterator const& other) const {
            return m_row == other.m_row &&
                    m_col == other.m_col &&
                m_mesh == other.m_mesh;
        }

        Patch dereference() const {
            return m_mesh->makePatch(m_col, m_row);
        }

    private:

        const Mesh* m_mesh;
        int m_col;
        int m_row;
    };

    class control_point_iterator :
        public boost::iterator_facade <control_point_iterator,
                                       QPointF,
                                       boost::bidirectional_traversal_tag>
    {
    public:
        control_point_iterator()
            : m_mesh(0),
              m_col(0),
              m_row(0),
              m_controlIndex(0)
        {}

        control_point_iterator(Mesh* mesh, int col, int row, int controlIndex)
            : m_mesh(mesh),
              m_col(col),
              m_row(row),
              m_controlIndex(controlIndex)
        {
        }

        Mesh::ControlType type() const {
            return Mesh::ControlType(m_controlIndex);
        }

        int col() const {
            return m_col;
        }

        int row() const {
            return m_row;
        }

        Mesh::ControlPointIndex controlIndex() const {
            return Mesh::ControlPointIndex(nodeIndex(), type());
        }

        Mesh::NodeIndex nodeIndex() const {
            return Mesh::NodeIndex(m_col, m_row);
        }

        Mesh::Node& node() const {
            return m_mesh->node(m_col, m_row);
        }

        bool isLeftBorder() const {
            return m_col == 0;
        }

        bool isRightBorder() const {
            return m_col == m_mesh->size().width() - 1;
        }

        bool isTopBorder() const {
            return m_row == 0;
        }

        bool isBottomBorder() const {
            return m_row == m_mesh->size().height() - 1;
        }


        bool isBorderNode() const {
            return isLeftBorder() || isRightBorder() || isTopBorder() || isBottomBorder();
        }

        bool isCornerNode() const {
            return (isLeftBorder() + isRightBorder() + isTopBorder() + isBottomBorder()) > 1;
        }

        bool isNode() const {
            return type() == Mesh::ControlPointIndex::Node;
        }

        control_point_iterator symmetricControl() const {
            typename Mesh::ControlPointIndex::ControlType newIndex =
                    Mesh::ControlPointIndex::Node;

            switch (type()) {
            case Mesh::ControlPointIndex::Node:
                newIndex = Mesh::ControlPointIndex::Node;
                break;
            case Mesh::ControlPointIndex::LeftControl:
                newIndex = Mesh::ControlPointIndex::RightControl;
                break;
            case Mesh::ControlPointIndex::RightControl:
                newIndex = Mesh::ControlPointIndex::LeftControl;
                break;
            case Mesh::ControlPointIndex::TopControl:
                newIndex = Mesh::ControlPointIndex::BottomControl;
                break;
            case Mesh::ControlPointIndex::BottomControl:
                newIndex = Mesh::ControlPointIndex::TopControl;
                break;
            }

            control_point_iterator it(m_mesh, m_col, m_row, newIndex);

            if (!it.controlIsValid()) {
                it = m_mesh->endControlPoints();
            }

            return it;
        }

        segment_iterator topSegment() const;
        segment_iterator bottomSegment() const;
        segment_iterator leftSegment() const;
        segment_iterator rightSegment() const;

        bool isValid() const {
            return nodeIsValid() && controlIsValid();
        }

    private:
        friend class boost::iterator_core_access;

        bool nodeIsValid() const {
            return m_col < m_mesh->size().width() && m_row < m_mesh->size().height();
        }

        bool controlIsValid() const {
            if (m_col == 0 && m_controlIndex == Mesh::ControlType::LeftControl) {
                return false;
            }

            if (m_col == m_mesh->m_size.width() - 1 && m_controlIndex == Mesh::ControlType::RightControl) {
                return false;
            }

            if (m_row == 0 && m_controlIndex == Mesh::ControlType::TopControl) {
                return false;
            }

            if (m_row == m_mesh->m_size.height() - 1 && m_controlIndex == Mesh::ControlType::BottomControl) {
                return false;
            }

            return true;
        }

        void increment() {
            do {
                m_controlIndex++;
                if (m_controlIndex > 4) {
                    m_controlIndex = 0;
                    m_col++;
                    if (m_col >= m_mesh->m_size.width()) {
                        m_col = 0;
                        m_row++;
                    }
                }
            } while (nodeIsValid() && !controlIsValid());
        }

        void decrement() {
            do {
                m_controlIndex--;
                if (m_controlIndex < 0) {
                    m_controlIndex = 4;
                    m_col--;
                    if (m_col < 0) {
                        m_col = m_mesh->m_size.width() - 1;
                        m_row--;
                    }
                }
            } while (nodeIsValid() && !controlIsValid());
        }


        bool equal(control_point_iterator const& other) const {
            return m_controlIndex == other.m_controlIndex &&
                m_row == other.m_row &&
                m_col == other.m_col &&
                m_mesh == other.m_mesh;
        }

        QPointF& dereference() const {
            return Mesh::ControlPointIndex::controlPoint(m_mesh->node(m_col, m_row), Mesh::ControlType(m_controlIndex));
        }

    private:

        Mesh* m_mesh;
        int m_col;
        int m_row;
        int m_controlIndex;
    };

    class segment_iterator :
        public boost::iterator_facade <segment_iterator,
                                       Mesh::SegmentIndex,
                                       boost::bidirectional_traversal_tag,
                                       Mesh::SegmentIndex>
    {
    public:
        segment_iterator()
            : m_mesh(0),
              m_col(0),
              m_row(0),
              m_isHorizontal(0)
        {}

        segment_iterator(Mesh* mesh, int col, int row, int isHorizontal)
            : m_mesh(mesh),
              m_col(col),
              m_row(row),
              m_isHorizontal(isHorizontal)
        {
        }

        Mesh::SegmentIndex segmentIndex() const {
            return
                 { Mesh::NodeIndex(m_col, m_row),
                   m_isHorizontal};
        }

        Mesh::NodeIndex firstNodeIndex() const {
            return Mesh::NodeIndex(m_col, m_row);
        }

        Mesh::NodeIndex secondNodeIndex() const {
            return m_isHorizontal ? Mesh::NodeIndex(m_col + 1, m_row) : Mesh::NodeIndex(m_col, m_row + 1);
        }

        Mesh::Node& firstNode() const {
            return m_mesh->node(firstNodeIndex());
        }

        Mesh::Node& secondNode() const {
            return m_mesh->node(secondNodeIndex());
        }

        QPointF& p0() const {
            return firstNode().node;
        }

        QPointF& p1() const {
            return m_isHorizontal ? firstNode().rightControl : firstNode().bottomControl;
        }

        QPointF& p2() const {
            return m_isHorizontal ? secondNode().leftControl : secondNode().topControl;
        }

        QPointF& p3() const {
            return secondNode().node;
        }

        control_point_iterator itP0() const {
            return m_mesh->find(firstNodeIndex());
        }

        control_point_iterator itP1() const {
            return m_mesh->find(ControlPointIndex(firstNodeIndex(),
                                                  m_isHorizontal ?
                                                      Mesh::ControlType::RightControl :
                                                      Mesh::ControlType::BottomControl));
        }

        control_point_iterator itP2() const {
            return m_mesh->find(ControlPointIndex(secondNodeIndex(),
                                                  m_isHorizontal ?
                                                      Mesh::ControlType::LeftControl :
                                                      Mesh::ControlType::TopControl));
        }

        control_point_iterator itP3() const {
            return m_mesh->find(secondNodeIndex());
        }

        int degree() const {
            return KisBezierUtils::bezierDegree(p0(), p1(), p2(), p3());
        }

        bool isHorizontal() const {
            return m_isHorizontal;
        }

        bool isValid() const {
            return nodeIsValid() && controlIsValid();
        }

    private:
        friend class boost::iterator_core_access;

        bool nodeIsValid() const {
            return m_col < m_mesh->size().width() && m_row < m_mesh->size().height();
        }

        bool controlIsValid() const {
            if (m_col == m_mesh->m_size.width() - 1 && m_isHorizontal) {
                return false;
            }

            if (m_row == m_mesh->m_size.height() - 1 && !m_isHorizontal) {
                return false;
            }

            return true;
        }

        void increment() {
            do {
                m_isHorizontal++;
                if (m_isHorizontal > 1) {
                    m_isHorizontal = 0;
                    m_col++;
                    if (m_col >= m_mesh->m_size.width()) {
                        m_col = 0;
                        m_row++;
                    }
                }
            } while (nodeIsValid() && !controlIsValid());
        }

        void decrement() {
            do {
                m_isHorizontal--;
                if (m_isHorizontal < 0) {
                    m_isHorizontal = 1;
                    m_col--;
                    if (m_col < 0) {
                        m_col = m_mesh->m_size.width() - 1;
                        m_row--;
                    }
                }
            } while (nodeIsValid() && !controlIsValid());
        }


        bool equal(segment_iterator const& other) const {
            return m_isHorizontal == other.m_isHorizontal &&
                m_row == other.m_row &&
                m_col == other.m_col &&
                m_mesh == other.m_mesh;
        }

        Mesh::SegmentIndex dereference() const {
            return segmentIndex();
        }

    private:

        Mesh* m_mesh;
        int m_col;
        int m_row;
        int m_isHorizontal;
    };

    patch_iterator beginPatches() const {
        return patch_iterator(this, 0, 0);
    }

    patch_iterator endPatches() const {
        return patch_iterator(this, 0, m_size.height() - 1);
    }

    // TODO: constness
    control_point_iterator beginControlPoints() {
        return control_point_iterator(this, 0, 0, ControlType::RightControl);
    }

    control_point_iterator endControlPoints() {
        return control_point_iterator(this, 0, m_size.height(), 0);
    }

    segment_iterator beginSegments() {
        return segment_iterator(this, 0, 0, 0);
    }

    segment_iterator endSegments() {
        return segment_iterator(this, 0, m_size.height(), 0);
    }

    QSize size() const {
        return m_size;
    }

    QRectF originalRect() const {
        return m_originalRect;
    }

    QRectF dstBoundingRect() const {
        QRectF result;
        for (auto it = beginPatches(); it != endPatches(); ++it) {
            result |= it->dstBoundingRect();
        }
        return result;
    }

    bool isIdentity() const {
        Mesh identityMesh(m_originalRect, m_size);
        return *this == identityMesh;
    }

    void translate(const QPointF &offset) {
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
            it->translate(offset);
        }
    }

    void transform(const QTransform &t) {
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
            it->transform(t);
        }
    }

    control_point_iterator hitTestPointImpl(const QPointF &pt, qreal distanceThreshold, bool onlyNodeMode) {
        const qreal distanceThresholdSq = pow2(distanceThreshold);

        control_point_iterator result = endControlPoints();
        qreal minDistanceSq = std::numeric_limits<qreal>::max();

        for (auto it = beginControlPoints(); it != endControlPoints(); ++it) {
            if (onlyNodeMode != (it.type() == ControlType::Node)) continue;

            const qreal distSq = kisSquareDistance(*it, pt);
            if (distSq < minDistanceSq && distSq < distanceThresholdSq) {
                result = it;
                minDistanceSq = distSq;
            }
        }

        return result;
    }

    control_point_iterator hitTestNode(const QPointF &pt, qreal distanceThreshold) {
        return hitTestPointImpl(pt, distanceThreshold, true);
    }

    control_point_iterator hitTestControlPoint(const QPointF &pt, qreal distanceThreshold) {
        return hitTestPointImpl(pt, distanceThreshold, false);
    }

    segment_iterator hitTestSegment(const QPointF &pt, qreal distanceThreshold, qreal *t = 0) {
        segment_iterator result = endSegments();
        qreal minDistance = std::numeric_limits<qreal>::max();

        for (auto it = beginSegments(); it != endSegments(); ++it) {

            qreal foundDistance = 0.0;
            const qreal foundT = KisBezierUtils::nearestPoint({it.p0(), it.p1(), it.p2(), it.p3()}, pt, &foundDistance);

            if (foundDistance < minDistance && foundDistance < distanceThreshold) {
                result = it;
                minDistance = foundDistance;
                if (t) {
                    *t = foundT;
                }
            }
        }

        return result;
    }

    patch_iterator hitTestPatch(const QPointF &pt, QPointF *localPointResult = 0) {
        const QRectF unitRect(0, 0, 1, 1);

        for (auto it = beginPatches(); it != endPatches(); ++it) {
            Patch patch = *it;

            if (patch.dstBoundingRect().contains(pt)) {
                const QPointF localPos = KisBezierUtils::calculateLocalPos(patch.points, pt);

                if (unitRect.contains(localPos)) {

                    if (localPointResult) {
                        *localPointResult = localPos;
                    }

                    return it;
                }
            }
        }
        return endPatches();

    }

    control_point_iterator find(const ControlPointIndex &index) {
        control_point_iterator it(this, index.nodeIndex.x(), index.nodeIndex.y(), index.controlType);
        return it.isValid() ? it : endControlPoints();
    }

    segment_iterator find(const SegmentIndex &index) {
        segment_iterator it(this, index.first.x(), index.first.y(), index.second);
        return it.isValid() ? it : endSegments();
    }

    patch_iterator findPatch(const PatchIndex &index) {
        patch_iterator it(this, index.x(), index.y());
        return it.isValid() ? it : endPatches();
    }

    void scaleForThumbnail(const QTransform &t) {
        KIS_SAFE_ASSERT_RECOVER_RETURN(t.type() <= QTransform::TxScale);
        transform(t);
        m_originalRect = t.mapRect(m_originalRect);
    }

protected:

    std::vector<Node> m_nodes;
    std::vector<qreal> m_rows;
    std::vector<qreal> m_columns;

    QSize m_size;
    QRectF m_originalRect;
};

template<typename Node, typename Patch>
QDebug operator<<(QDebug dbg, const Mesh<Node, Patch> &mesh)
{
    dbg.nospace() << "Mesh " << mesh.size() << "\n";

    for (int y = 0; y < mesh.size().height(); y++) {
        for (int x = 0; x < mesh.size().width(); x++) {
            dbg.nospace() << "    node(" << x << ", " << y << ") " << mesh.node(x, y) << "\n";
        }
    }
    return dbg.space();
}

template<typename NodeArg, typename PatchArg>
typename Mesh<NodeArg, PatchArg>::segment_iterator
Mesh<NodeArg, PatchArg>::patch_iterator::segmentP() const
{
    // FIXME: remove const cast
    return segment_iterator(const_cast<Mesh<NodeArg, PatchArg>*>(m_mesh), m_col, m_row, 1);
}

template<typename NodeArg, typename PatchArg>
typename Mesh<NodeArg, PatchArg>::segment_iterator
Mesh<NodeArg, PatchArg>::patch_iterator::segmentQ() const
{
    // FIXME: remove const cast
    return segment_iterator(const_cast<Mesh<NodeArg, PatchArg>*>(m_mesh), m_col, m_row + 1, 1);
}

template<typename NodeArg, typename PatchArg>
typename Mesh<NodeArg, PatchArg>::segment_iterator
Mesh<NodeArg, PatchArg>::patch_iterator::segmentR() const
{
    // FIXME: remove const cast
    return segment_iterator(const_cast<Mesh<NodeArg, PatchArg>*>(m_mesh), m_col, m_row, 0);
}

template<typename NodeArg, typename PatchArg>
typename Mesh<NodeArg, PatchArg>::segment_iterator
Mesh<NodeArg, PatchArg>::patch_iterator::segmentS() const
{
    // FIXME: remove const cast
    return segment_iterator(const_cast<Mesh<NodeArg, PatchArg>*>(m_mesh), m_col + 1, m_row, 0);
}

template<typename NodeArg, typename PatchArg>
typename Mesh<NodeArg, PatchArg>::segment_iterator
Mesh<NodeArg, PatchArg>::control_point_iterator::topSegment() const
{
    if (isTopBorder()) {
        return m_mesh->endSegments();
    }

    return segment_iterator(m_mesh, m_col, m_row - 1, false);
}

template<typename NodeArg, typename PatchArg>
typename Mesh<NodeArg, PatchArg>::segment_iterator
Mesh<NodeArg, PatchArg>::control_point_iterator::bottomSegment() const
{
    if (isBottomBorder()) {
        return m_mesh->endSegments();
    }

    return segment_iterator(m_mesh, m_col, m_row, false);
}

template<typename NodeArg, typename PatchArg>
typename Mesh<NodeArg, PatchArg>::segment_iterator
Mesh<NodeArg, PatchArg>::control_point_iterator::leftSegment() const
{
    if (isLeftBorder()) {
        return m_mesh->endSegments();
    }

    return segment_iterator(m_mesh, m_col - 1, m_row, true);
}

template<typename NodeArg, typename PatchArg>
typename Mesh<NodeArg, PatchArg>::segment_iterator
Mesh<NodeArg, PatchArg>::control_point_iterator::rightSegment() const
{
    if (isRightBorder()) {
        return m_mesh->endSegments();
    }

    return segment_iterator(m_mesh, m_col, m_row, true);
}

template<typename NodeArg, typename PatchArg>
void smartMoveControl(Mesh<NodeArg, PatchArg> &mesh,
                      typename Mesh<NodeArg, PatchArg>::ControlPointIndex index,
                      const QPointF &move,
                      bool lockNodes)
{
    using ControlType = typename Mesh<NodeArg, PatchArg>::ControlType;
    using ControlPointIndex = typename Mesh<NodeArg, PatchArg>::ControlPointIndex;

    auto it = mesh.find(index);
    KIS_SAFE_ASSERT_RECOVER_RETURN(it != mesh.endControlPoints());

    if (it.isNode()) {
        it.node().translate(move);
    } else {
        const QPointF newPos = *it + move;

        if (lockNodes) {
            const qreal rotation = KisAlgebra2D::angleBetweenVectors(*it - it.node().node,
                                                                     newPos - it.node().node);
            QTransform R;
            R.rotateRadians(rotation);

            const QTransform t =
                    QTransform::fromTranslate(-it.node().node.x(), -it.node().node.y()) *
                    R *
                    QTransform::fromTranslate(it.node().node.x(), it.node().node.y());

            for (int intType = 0; intType < 4; intType++) {
                ControlType type = static_cast<ControlType>(intType);

                if (type == ControlType::Node ||
                    type == index.controlType) {

                    continue;
                }

                auto neighbourIt = mesh.find(ControlPointIndex(index.nodeIndex, type));
                if (neighbourIt == mesh.endControlPoints()) continue;

                *neighbourIt = t.map(*neighbourIt);
            }
        }

        *it = newPos;
    }
}

KRITAGLOBAL_EXPORT
QDebug operator<<(QDebug dbg, const BaseMeshNode &n);

KRITAGLOBAL_EXPORT
void saveValue(QDomElement *parent, const QString &tag, const BaseMeshNode &node);

KRITAGLOBAL_EXPORT
bool loadValue(const QDomElement &parent, BaseMeshNode *node);

}

namespace KisDomUtils {
using KisBezierMeshDetails::loadValue;
using KisBezierMeshDetails::saveValue;
}

template <typename Node, typename Patch>
using KisBezierMeshBase = KisBezierMeshDetails::Mesh<Node, Patch>;

using KisBezierMesh = KisBezierMeshDetails::Mesh<KisBezierMeshDetails::BaseMeshNode, KisBezierPatch>;


#endif // KISBEZIERMESH_H
