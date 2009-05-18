/*  Render HTML page, write out as PNG
 *  Heavily based on KDE HTML thumbnail creator
 *  Copyright (C) 2003 Simon MacMullen
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

class KHTMLPart;

class KHTML2PNG : public QObject
{
    Q_OBJECT
public:
    KHTML2PNG();
    ~KHTML2PNG();
    QImage* create(const QString &path, int width, int height, int time, int flashDelay);

protected:
    virtual void timerEvent(QTimerEvent *);

private slots:
    void slotCompleted();

private:
    KHTMLPart *m_html;
    bool m_completed;
    bool m_flashStarted;
};

#endif
