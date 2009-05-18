/*  Render HTML page, write out as PNG
    Heavily based on KDE HTML thumbnail creator
    Copyright (C) 2003 Simon MacMullen
    Copyright (C) 2004-2006 Hauke Goos-Habermann
    Copyright (C) 2007 Florent Bruneau

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; see the file COPYING.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
    */

#include <qpixmap.h>
#include <qimage.h>
#include <qpainter.h>
#include <qobjectlist.h>
#include <qtimer.h>

#include <khtml_part.h>
#include <khtmlview.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kaboutdata.h>

#include <dom/html_misc.h> //<-- use this for Mandriva
//#include <kde/dom/html_misc.h> //<-- use this for other distributions

#include "khtml2png.h"





/**
 **name KHTML2PNG(const QString& path, const QString& id, int m_width, int m_height)
 **description Start a new instance
 **parameter path: URL to open
 **parameter id: ID for autodetection
 **parameter width: Width of the screenshot (if id is empty)
 **parameter height: Height of the screenshot (if id is empty)
 **/
KHTML2PNG::KHTML2PNG(const KCmdLineArgs* const args)
:KApplication(), m_html(0), pix(0)
{
	const QString width  = args->getOption("width");
	const QString height = args->getOption("height");
	autoDetectId = args->getOption("auto");
	timeoutMillis = args->getOption("time").toUInt() * 1000;

	rect = QRect(0, 0, width.isEmpty() ? -1 : width.toInt(), height.isEmpty() ? -1 : height.toInt());

	detectionCompleted = false;
	loadingCompleted   = false;
	filename           = args->arg(1);
	killPopup = !args->isSet("disable-popupkiller");
	init(args->arg(0), !args->isSet("disable-js"),
	     !args->isSet("disable-java"),
	     !args->isSet("disable-plugins"),
	     !args->isSet("disable-redirect"));
}





/**
 **name ~KHTML2PNG()
 **description: Destructor
 **/
KHTML2PNG::~KHTML2PNG()
{
	if (m_html)
	{
		delete m_html;
	}
	if (pix)
	{
		delete pix;
	}
}





/**
 **name eventFilter(QObject *o, QEvent *e)
 **description Intercept QMessageBoxes creation and delete them in order keep a non-modal interface
 **/
bool KHTML2PNG::eventFilter(QObject *o, QEvent *e)
{
	if (e->type() == QEvent::ChildInserted && killPopup)
	{
		QChildEvent *ce = (QChildEvent*)e;
		if (ce->child()->inherits("QDialog"))
		{
			o->removeChild(ce->child());
			((QDialog*)(ce->child()))->setModal(false);
			ce->child()->deleteLater();
		}
	}
	return false;
}





/**
 **name grabChildWidgets( QWidget * w )
 **description Creates a screenshot with all widgets of a window.
 **parameter w: Pointer to the window widget.
 **returns: QPixmap with the screenshot.
 **/
QPixmap *KHTML2PNG::grabChildWidgets(QWidget* w) const
{
	/*
	   This solution was taken from:
http://lists.kde.org/?l=kde-devel&m=108664293315286&w=2
*/
	w->repaint(false);
	QPixmap *res = new QPixmap(w->width(), w->height());
	if (w->rect().isEmpty())
	{
		return res;
	}
	res->fill(w, QPoint(0, 0));
	::bitBlt(res, QPoint(0, 0), w, w->rect(), Qt::CopyROP, true);

	const QObjectList *children = w->children();
	if (!children)
	{
		return res;
	}
	QPainter p(res, true);
	QObjectListIterator it(*children);
	QObject *child;
	while ((child = it.current()) != 0)
	{
		++it;
		if (child->isWidgetType()
		    && ((QWidget *)child)->geometry().intersects(w->rect())
		    && !child->inherits("QDialog"))
		{

			// those conditions aren't quite right, it's possible
			// to have a grandchild completely outside its
			// grandparent, but partially inside its parent.  no
			// point in optimizing for that.
			const QPoint childpos = ((QWidget *)child)->pos();
			const QPixmap * const cpm = grabChildWidgets( (QWidget *)child );

			if (cpm->isNull())
			{
				// Some child pixmap failed - abort and reset
				res->resize(0, 0);
				delete cpm;
				break;
			}

			p.drawPixmap(childpos, *cpm);
			p.flush();
			delete cpm;
		}
	}
	return res;
}




/**
 **name resizeClipper(const int width, const int height)
 **description Try to resize the khtmlview so that the visible area will have at least the given size
 **/
void KHTML2PNG::resizeClipper(const int width, const int height)
{
	const int x = width + m_html->view()->width() - m_html->view()->clipper()->width();
	const int y = height + m_html->view()->height() - m_html->view()->clipper()->height();
	m_html->view()->resize(x, y);
}




/**
 **name slotCompleted()
 **description Searches for the position of a HTML element to use as screenshot size marker or sets the m_completed variable.
 **/
void KHTML2PNG::completed()
{
	loadingCompleted = true;
	if (!detectionCompleted && !autoDetectId.isEmpty())
	{
		//search for the HTML element
		DOM::Node markerNode = m_html->htmlDocument().all().namedItem(autoDetectId);

		if (!markerNode.isNull())
		{
			//get its position
			rect = m_html->htmlDocument().all().namedItem(autoDetectId).getRect();
			if (rect.isEmpty()) {
				rect = QRect(0, 0, rect.right(), rect.bottom());
			}
			resizeClipper(rect.right() + 200, rect.bottom() + 200);
			rect = m_html->htmlDocument().all().namedItem(autoDetectId).getRect();
			if (rect.isEmpty()) {
				rect = QRect(0, 0, rect.right(), rect.bottom());
			}
			detectionCompleted = true;
		}
		else
		{
			fprintf(stderr,
				"ERROR: Can't find a HTML element with the ID \"%s\" in the current page.\n",
				autoDetectId.latin1());
			autoDetectId = QString::null;
		}
	}
	doRendering();
	if (save()) {
		quit();
	}
	exit(1);
}





/**
 **name openURLRequest(const KURL &url, const KParts::URLArgs & )
 **description Used to change the chosen url (needed for navigation on the page e.g. clicking on links).
 **parameter url: the URL to the HTML document
 **parameter URLArgs: standard parameter for KParts
 **/
void KHTML2PNG::openURLRequest(const KURL &url, const KParts::URLArgs & )
{
	m_html->openURL(url.url());
}





/**
 **name init(const QString& path)
 **description Creates the needed KHTMLPart object for the browser and connects signals and slots.
 **parameter path: URL to open
 **/
void KHTML2PNG::init(const QString& path, const bool js, const bool java, const bool plugins, const bool redirect)
{
	m_html = new KHTMLPart;
	m_html->view()->installEventFilter(this);

	//set some basic settings
	m_html->setJScriptEnabled(js);
	m_html->setJavaEnabled(java);
	m_html->setPluginsEnabled(plugins);
	m_html->setMetaRefreshEnabled(redirect);
	m_html->setOnlyLocalReferences(false);
	m_html->setAutoloadImages(true);
	m_html->view()->setResizePolicy(QScrollView::Manual);
	m_html->view()->setHScrollBarMode(QScrollView::AlwaysOff);
	m_html->view()->setVScrollBarMode(QScrollView::AlwaysOff);

	//this is needed for navigation on the page e.g. clicking on links
	connect(m_html->browserExtension(),
		SIGNAL(openURLRequestDelayed(const KURL&, const KParts::URLArgs&)),this,
		SLOT(openURLRequest(const KURL&, const KParts::URLArgs&)));
	connect(m_html, SIGNAL(completed()),this,SLOT(completed()));

	//at the beginning the loading isn't completely
	loadingCompleted = false;

	//show the window
	m_html->view()->move(0, 0);
	m_html->view()->showMaximized();
	processEvents(200);
	xVisible = m_html->view()->clipper()->width() - 20;
	yVisible = m_html->view()->clipper()->height() - 20;

	// set a maximum time before we just snapshot whatever we've got loaded so far
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(completed()));
	timer->start(timeoutMillis, false);

	m_html->openURL(path);
}





/**
 **name doRendering()
 **description Take a snapshot of the browser window.
 **/
void KHTML2PNG::doRendering()
{
	int yLimit = rect.bottom();
	int xLimit = rect.right();

	pix = new QPixmap(rect.width(), rect.height());
	pix->fill();

	if (autoDetectId.isEmpty())
	{
		if (rect.width() != m_html->view()->clipper()->width() || rect.height() != m_html->view()->clipper()->height()) {
			m_html->view()->resize(rect.width(), rect.height());
			processEvents(200);
			resizeClipper(rect.width(), rect.height());
		}
		int bottom = m_html->htmlDocument().getRect().bottom();
		if (bottom < yLimit) {
			yLimit = bottom;
		}
		int right = m_html->htmlDocument().getRect().right();
		if (right < xLimit) {
			xLimit = right;
		}
	}

	xVisible = (rect.width() < xVisible ? rect.width() : xVisible);
	yVisible = (rect.height() < yVisible ? rect.height() : yVisible);

	const QPoint clipperPos = m_html->view()->clipper()->pos();
	for (int yPos = rect.top() ; yPos <= yLimit ; yPos += yVisible)
	{
		for (int xPos = rect.left() ; xPos <= xLimit ; xPos += xVisible)
		{
			m_html->view()->move(-xPos - clipperPos.x(), -yPos - clipperPos.y());
			m_html->view()->repaint();
			processEvents(200);

			//capture the part of the screen
			const QPixmap* const temp = grabChildWidgets(m_html->view()->clipper());
			QRect pos = temp->rect();
			pos.setLeft(pos.left() + xPos);
			pos.setTop(pos.top() + yPos);
			::bitBlt(pix, QPoint(xPos - rect.left(), yPos - rect.top()), temp, pos, Qt::CopyROP, true);
			delete temp;
		}
	}
}





/**
 **name save(const QString& file)
 **description Save the snapshot in a file
 **parameter file: filename
 **returns true, if the saving was sucessfully otherwise false.
 **/
bool KHTML2PNG::save() const
{
	QString format = filename.section('.', -1).stripWhiteSpace().upper();
	if (format == "JPG" || format == "JPE")
	{
		format = "JPEG";
	}
	return pix->convertToImage().save(filename, format);
}





/**
 **name options
 **description Array with command line options and descriptions
 **/
static KCmdLineOptions options[] =
{
	{ "w", 0, 0},
	{ "width <width>", "Width of canvas on which to render html", "800" },
	{ "h", 0, 0},
	{ "height <height>", "Height of canvas on which to render html", "1000" },
	{ "t", 0, 0},
	{ "time <time>", "Maximum time in seconds to spend loading page", "30" },
	{ "auto <id>", "Use this option if you to autodetect the bottom/right border", "" },
	{ "disable-js", "Enable/Disable javascript (enabled by default)", 0 },
	{ "disable-java", "Enable/Disable java (enabled by default)", 0},
	{ "disable-plugins", "Enable/Disable KHTML plugins (like Flash player, enabled by default)", 0},
	{ "disable-redirect", "Enable/Disable auto-redirect by header <meta > (enabled by default)", 0},
	{ "disable-popupkiller", "Enable/Disable popup auto-kill (enabled by default)", 0},
	{ "+url ", "URL of page to render", 0 },
	{ "+outfile ", "Output file", 0 },
	{ 0, 0, 0 }
};





int main(int argc, char **argv)
{
	KAboutData aboutData("khtml2png", I18N_NOOP("KHTML2PNG"), "2.6.7",
			     I18N_NOOP("Render HTML to a PNG from the command line\n\
				       Example:\n\
				       khtml2png2 --width 800 --height 1000 http://www.kde.org/ kde-org.png\n\
				       or\n\
				       khtml2png --auto ID_border http://www.kde.org/ kde-org.png"),
			     KAboutData::License_GPL,
			     "(c) 2003 Simon MacMullen, 2004-2007 Hauke Goos-Habermann, 2007 Florent Bruneau, 2007 Alex Osborne");
	aboutData.addAuthor("Simon MacMullen", 0, "s.macmullen@ry.com");
	aboutData.addAuthor("Hauke Goos-Habermann", 0, "hhabermann@pc-kiel.de","http://khtml2png.sourceforge.net");
	aboutData.addAuthor("Florent Bruneau", 0, "florent.bruneau@m4x.org", "http://fruneau.rznc.net");
	aboutData.addAuthor("Alex Osborne", 0, "alex@mugofgrog.com", "http://www.mugofgrog.com");
	KCmdLineArgs::init(argc, argv, &aboutData);
	KCmdLineArgs::addCmdLineOptions(options);
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

	if (args->count() < 2)
	{
		args->usage();
		exit(1);
	}

	KInstance inst(&aboutData);
	KHTML2PNG app(args);
	app.exec(); 
}

#include "khtml2png.moc"

// vim:set noet sw=8 sts=8 ts=8:
