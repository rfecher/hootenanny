/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. Maxar
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2016, 2017, 2021 Maxar (http://www.maxar.com/)
 */
package hoot.services.controllers.review;


/**
 * Response to a review references web request
 */
public class ReviewRefsResponse {
    private ReviewRef[] reviewRefs;
    private ElementInfo queryElementInfo;

    public ReviewRefsResponse() {}

    public ReviewRefsResponse(ElementInfo requestingElementInfo, ReviewRef[] reviewReferences) {
        this.queryElementInfo = requestingElementInfo;
        this.reviewRefs = reviewReferences;
    }

    public ReviewRef[] getReviewRefs() {
        return reviewRefs;
    }

    public void setReviewRefs(ReviewRef[] refs) {
        this.reviewRefs = refs;
    }

    public ElementInfo getQueryElementInfo() {
        return queryElementInfo;
    }

    public void setQueryElementInfo(ElementInfo info) {
        this.queryElementInfo = info;
    }

    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder("Request element ").append(queryElementInfo)
                        .append(", Review references:").append(System.lineSeparator());
        for (ReviewRef reviewRef : reviewRefs) {
            stringBuilder.append(reviewRef).append(System.lineSeparator());
        }
        return stringBuilder.toString();
    }
}