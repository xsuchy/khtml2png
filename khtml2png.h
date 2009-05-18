/*  Render HTML page, write out as PNG
 *  Heavily based on KDE HTML thumbnail creator
 *  Copyright (C) 2003 Simon MacMullen
 *  Copyright (C) 2004-2006 Hauke Goos-Habermann
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
	QImage* create(const QString &path/*, int flashDelay*/);
	void showMiniBrowser(const QString &path);

public slots:
	void openURLRequest(const KURL &url, const KParts::URLArgs & );
	void completed();

protected:
	virtual void timerEvent(QTimerEvent *);
	void create_m_html();
	QPixmap grabChildWidgets( QWidget * w );

private slots:
	void slotCompleted();

private:
	bool m_flashStarted;
	bool m_completed;
	bool browser;
	bool loadingCompleted; //indicates is the page is loaded completely
};

#endif
