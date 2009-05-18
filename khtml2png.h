/*  Render HTML page, write out as PNG
 *  Heavily based on KDE HTML thumbnail creator
 *  Copyright (C) 2003 Simon MacMullen
 *  Copyright (C) 2004-2006 Hauke Goos-Habermann
 *  Copyright (C) 2007 Florent Bruneau
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 */

#ifndef _KHTML2PNG_H_
#define _KHTML2PNG_H_

#include <kapplication.h>

class KHTMLPart;
class KCmdLineArgs;

class KHTML2PNG : public KApplication
{
    Q_OBJECT

    KHTMLPart *m_html;
    bool m_completed;
    bool browser;
    bool loadingCompleted; //indicates if the page is loaded completely
    bool detectionCompleted;
    bool killPopup;
    bool show;

    QString autoDetectId;
    QString filename;
    QRect   rect;
    QSize   scaled;
    QPixmap *pix;

    int xVisible;
    int yVisible;

    uint timeoutMillis; // maximum milliseconds to wait for page to load

    public:
        KHTML2PNG(const KCmdLineArgs* const args);
        ~KHTML2PNG();
        bool save() const;

    protected:
        virtual bool eventFilter(QObject *o, QEvent *e);

    private:
        void init(const QString& path, const bool js = true, 
                                       const bool java = true,
                                       const bool plugins = true,
                                       const bool redirect = true);
        QPixmap *grabChildWidgets(QWidget *w) const;
        void doRendering();
        void resizeClipper(const int width, const int height);

    private slots:
        void openURLRequest(const KURL &url, const KParts::URLArgs & );
        void completed();
};

#endif

// vim:set et sw=4 sts=4 sws=4:
