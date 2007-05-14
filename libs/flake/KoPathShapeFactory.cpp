/* This file is part of the KDE project
 * Copyright (C) 2006 Thomas Zander <zander@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include <klocale.h>

#include <KoXmlReader.h>
#include <KoXmlNS.h>

#include "KoPathShapeFactory.h"
#include "KoPathShape.h"
#include "KoLineBorder.h"

KoPathShapeFactory::KoPathShapeFactory(QObject *parent, const QStringList&)
    : KoShapeFactory(parent, KoPathShapeId, i18n("A simple path shape"))
{
    setToolTip("A simple path shape");
    setIcon("pathshape");
    setOdfElementNames(KoXmlNS::draw, QStringList( QString( "path") ) );
    setLoadingPriority( 0 );
}

KoShape * KoPathShapeFactory::createDefaultShape() const {
    KoPathShape* path = new KoPathShape();
    path->moveTo( QPointF( 0, 50 ) );
    path->curveTo( QPointF( 0, 120 ), QPointF( 50, 120 ), QPointF( 50, 50 ) );
    path->curveTo( QPointF( 50, -20 ), QPointF( 100, -20 ), QPointF( 100, 50 ) );
    path->normalize();
    path->setBorder( new KoLineBorder( 1.0 ) );
    return path;
}

KoShape * KoPathShapeFactory::createShape(const KoProperties * params) const {
    Q_UNUSED(params);
    return new KoPathShape();
}

bool KoPathShapeFactory::supports(const KoXmlElement & e) const
{
    return ( e.localName() == "path" && e.namespaceURI() == KoXmlNS::draw );
}

