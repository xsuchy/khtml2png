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

class myKHTMLPart : public KHTMLPart {
	virtual void showError( KIO::Job* job );
//	friend class KHTML2PNG;
};

class KHTML2PNG : public QObject
{
    Q_OBJECT
public:
    KHTML2PNG(int jscript, int java, int plugin, int refresh, int localonly);
    virtual ~KHTML2PNG() {delete m_html;}
    QImage* create(const QString &path, int width, int height, int time, int flashDelay);
	void processBatch(const QString &fileName, const QString &path, int width, int height, int time, int flashDelay, int scaledWidth, int scaledHeight);
	QString lastModified();
	bool haveFlash();

protected:
    virtual void timerEvent(QTimerEvent *);
	void showTree(const DOM::Node &pNode);
	void showRecursive(const DOM::Node &node);


private slots:
    void slotCompleted();

	
private:
	myKHTMLPart *m_html;
	bool flashflag;
	bool m_completed;
	bool m_flashStarted;
};


#endif
