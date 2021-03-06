/*
 *  Copyright (c) 2008 Lukáš Tvrdý (lukast.dev@gmail.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef EXPERIMENT_PAINTOP_PLUGIN_H_
#define EXPERIMENT_PAINTOP_PLUGIN_H_

#include <QObject>
#include <QVariant>

/**
 * A plugin wrapper that adds the paintop factories to the paintop registry.
 */
class ExperimentPaintOpPlugin : public QObject
{
    Q_OBJECT
public:
    ExperimentPaintOpPlugin(QObject *parent, const QVariantList &);
    ~ExperimentPaintOpPlugin() override;
};

#endif // EXPERIMENT_PAINTOP_PLUGIN_H_
