/*
 * Copyright 2017, 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <opencv2/core/core.hpp>

// Line description y = alpha * x + beta
struct Line
{
    double alpha;
    double beta;
};

/** @brief Calculates line between 2 points
  * If line is x = X, then alpha = INFINITY and beta = X.
  */
static inline Line GetAlphaBeta(cv::Point2f p1, cv::Point2f p2)
{
    Line result;
    if (p1.x == p2.x) {
        // If line is x = X, then alpha = INFINITY and beta = X.
        result.alpha = INFINITY;
        result.beta = p1.x;
    } else {
        result.alpha = ((double)p2.y - (double)p1.y) / ((double)p2.x - (double)p1.x);
        result.beta = ((double)p1.y * p2.x - (double)p2.y * p1.x) / ((double)p2.x - (double)p1.x);
    }
    return result;
}

/** @brief Calculate lines intersection point
  * If lines are parallel the INFINITY value is return.
  * If alpha coefficient of a line is equal INFINITY, then line is x = beta.
  */
static inline cv::Point2f GetIntersection(Line line1, Line line2)
{
    cv::Point2f result;
    if (line1.alpha == line2.alpha) {
        // Lines are parallel
        result.x = INFINITY;
        result.y = INFINITY;
    } else if (line1.alpha == INFINITY) {
        // First line is x = const
        result.x = (float)line1.beta;
        result.y = (float)(line2.alpha * result.x + line2.beta);
    } else if (line2.alpha == INFINITY) {
        // Second line is x = const
        result.x = (float)line2.beta;
        result.y = (float)(line1.alpha * result.x + line1.beta);
    } else {
        result.x = (float)((line2.beta - line1.beta) / (line1.alpha - line2.alpha));
        result.y = (float)((line1.alpha * line2.beta - line2.alpha * line1.beta) / (line1.alpha - line2.alpha));
    }
    return result;
}
