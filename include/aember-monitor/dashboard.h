/**
 * @file dashboard.h
 * @brief Draws one frame of the monitor with QPainter (raster, no OpenGL).
 *
 * Pure: the same sample and size always give the same picture, so tests
 * render into a QImage. Everything scales with the target size (from a
 * 320x240 panel to a full HD screen).
 */
#pragma once

#include <QPainter>
#include <QSize>
#include <QString>
#include <optional>

#include <aember-monitor/model.h>

namespace aember::monitor {

/// @p status is shown when there is no sample (e.g. "waiting for
/// aember-stats").
void Paint(QPainter& painter, QSize size, const std::optional<Sample>& sample,
           const QString& status);

}  // namespace aember::monitor
