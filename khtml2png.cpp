/*  Render HTML page, write out as PNG
	Heavily based on KDE HTML thumbnail creator
	Copyright (C) 2003 Simon MacMullen
	Copyright (C) 2004-2006 Hauke Goos-Habermann

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

#include <time.h>

#include <qpixmap.h>
#include <qimage.h>
#include <qpainter.h>
#include <qobjectlist.h>

#include <kapplication.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <stdlib.h>

//#include <dom/html_misc.h> //<-- use this for Mandriva
#include <kde/dom/html_misc.h> //<-- use this for other distributions

#include "khtml2png.h"

KHTMLPart *m_html;
bool	lastYShot = false,
		lastXShot = false;

//capture coordinates
int xOrig,
	xRemain,
	yRemain,
	xPos=0,
	yPos=0,
	xVisible,
	yVisible,
	xCapture,
	yCapture;

//set to true if the capture size sould be autodetected
bool autoDetect;
//ID of the HTML element to use as bottom right border for the screenshot
QString autoDetectItemName;
bool detectionCompleted=false;





/**
**name grabChildWidgets( QWidget * w )
**description Creates a screenshot with all widgets of a window.
**parameter w: Pointer to the window widget.
**returns: QPixmap with the screenshot.
**/
QPixmap KHTML2PNG::grabChildWidgets( QWidget * w )
{
	/*
		This solution was taken from:
		http://lists.kde.org/?l=kde-devel&m=108664293315286&w=2
	*/
	char fn[50];
	int x=w->width(),
		y=w->height();
	
	kapp->postEvent( w, new QPaintEvent( w->rect(), Qt::WRepaintNoErase));
	kapp->processOneEvent();
	QPixmap res( w->width(), w->height() );
	if ( res.isNull() && w->width() )
		return res;
	res.fill( w, QPoint( 0, 0 ) );
	::bitBlt( &res, QPoint(0, 0), w, w->rect(), Qt::CopyROP, true );

	const QObjectList * children = w->children();
	if ( children ) {
		QPainter p( &res, TRUE );
		QObjectListIt it( *children );
		QObject * child;
		while( (child=it.current()) != 0 ) {
			++it;
			if ( child->isWidgetType() &&
				 ((QWidget *)child)->geometry().intersects( w->rect() ) &&
				 ! child->inherits( "QDialog" ) ) {

				// those conditions aren't quite right, it's possible
				// to have a grandchild completely outside its
				// grandparent, but partially inside its parent.  no
				// point in optimizing for that.

				// make sure to evaluate pos() first - who knows what
				// the paint event(s) inside grabChildWidgets() will do.
				QPoint childpos = ((QWidget *)child)->pos();
				QPixmap cpm = grabChildWidgets( (QWidget *)child );
				
				if ( cpm.isNull() ) {
					// Some child pixmap failed - abort and reset
					res.resize( 0, 0 );
					break;
				}
			   
				p.drawPixmap( childpos, cpm);
				p.flush();
			}
		}
	}
	return res;
}





/**
**name slotCompleted()
**description Searches for the position of a HTML element to use as screenshot size marker or sets the m_completed variable.
**/
void KHTML2PNG::slotCompleted()
{
	if (detectionCompleted)
		m_completed = true;
	else
	{
		//search for the HTML element
		DOM::Node markerNode=m_html->htmlDocument().all().namedItem(autoDetectItemName);
	
		if (!detectionCompleted && !markerNode.isNull())
		{
			//get its position
			QRect rec = markerNode.getRect();
			xOrig = rec.left();
			yRemain = rec.top();
			detectionCompleted = true;
		}
		else if (autoDetect)
		{
			fprintf(stderr,"ERROR: Can't find a HTML element with the ID \"%s\" in the current page.\n",autoDetectItemName.latin1());
			exit(3);
		};
	};
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
**name completed()
**description Is connected to the completed() signal of KParts::ReadOnlyPart that is emitted when the page is loaded completely. This procedure makes the "loading finished" status available for the whole programm.
**/
void KHTML2PNG::completed()
{
	loadingCompleted=true;
}





/**
**name create_m_html()
**description Creates the needed KHTMLPart object for the browser and connects signals and slots.
**/
void KHTML2PNG::create_m_html()
{
	m_html = new KHTMLPart;
	connect(m_html, SIGNAL(completed()), SLOT(slotCompleted()));

	//set some basic settings
	m_html->setJScriptEnabled(true);
	m_html->setJavaEnabled(true);
	m_html->setPluginsEnabled(true);
	m_html->setMetaRefreshEnabled(false);
	m_html->setOnlyLocalReferences(false);
	m_html->setAutoloadImages(true);
	m_html->view()->setResizePolicy(QScrollView::Manual);

	//this is needed for navigation on the page e.g. clicking on links
	connect( m_html->browserExtension(),
	SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
	this, SLOT( openURLRequest(const KURL &, const KParts::URLArgs & ) ) );
	connect( m_html,
	SIGNAL( completed() ),this, SLOT( completed() ));

	//at the beginning the loading isn't completely
	loadingCompleted=false;

	//show the window
	m_html->view()->show();
	kapp->processOneEvent();
}





/**
**name showMiniBrowser(const QString &path)
**description Shows a window with minimal browser.
**parameter path: the URL to the HTML document
**/
void KHTML2PNG::showMiniBrowser(const QString &path)
{
	create_m_html();
	m_html->openURL(path);

	//make sure the page is loaded completely
	while (!loadingCompleted)
		kapp->processOneEvent();
	
	m_html->view()->resize(xOrig, yRemain);
	kapp->processOneEvent();

	while (!detectionCompleted)
		kapp->processOneEvent();
};





/**
**name create(const QString &path, int flashDelay)
**description Creates a minimal browser that scrolls, resizes and makes a screenshot.
**parameter path: the URL to the HTML document
**parameter flashDelay: time in seconds to wait before making the screenshot
**parameter xCapture (global): resize width of the window
**parameter yCapture (global): resize height of the window
**parameter xPos (global): left position to scroll to 
**parameter yPos (global): top position to scroll to
**returns the visible part as a pointer QImage
**/
QImage* KHTML2PNG::create(const QString &path/*, int flashDelay*/)
{
	int visibleX,visibleY;
	QPixmap pix;

	if (!m_html)
	{
		int maxX=0, //size of the HTML document
			maxY=0,
			sizeDiffX=0,
			sizeDiffY=0;

		/*
			if the KHTMLPart object doesn't exist, create it
			set some basic HTML settings
			and open the window
		*/
		create_m_html();
		
		//open the HTMl page in the window
		m_html->openURL(path);

		//make sure the page is loaded completely
		while (!loadingCompleted)
			kapp->processOneEvent();

		m_html->view()->setMarginWidth(0);
		m_html->view()->setMarginHeight(0);

		//resize the window to the wanted size
		m_html->view()->resize(xCapture, yCapture);
		kapp->processOneEvent();

		//stop the animations
		m_html->stopAnimations();
		kapp->processOneEvent();

		//make a test screenshot to check what visible size we've got
		pix = grabChildWidgets(m_html->view()->clipper());
		visibleX=pix.width();
		visibleY=pix.height();

		/*
			calculate the difference and adjust the capture size only if it's a small difference

			annotation: small values normally mean the difference between the window size and the
			window area that contains the HTML page. The visible area is reduced e.g. by scrollbars
			or the window title.
		*/
		sizeDiffX = xCapture-visibleX;
		if (sizeDiffX < 20)
			xCapture+=sizeDiffX;
		sizeDiffY = yCapture-visibleY;
		if (sizeDiffY < 20)
			yCapture+=sizeDiffY;

		//set the new capture size (we get hopefully the correct size *cross your fingers*)
		m_html->view()->resize(xCapture, yCapture);
		kapp->processOneEvent();

		/*
			annotation: There seem to be lots of bugs in the KDElibs and underlaying QT functions.
				-> m_html->view()->contentsHeight(): doesn't return a correct height (more than 2x of the correct value)
				-> m_html->view()->horizontalScrollBar()->maxValue(): returns 0
				
			That's why all HTML elements in the document are searched. The most right point of an element is assumed as the
			document's width and the most bottom as the height.
		*/
		DOM::Node nodeJumper=m_html->htmlDocument().all().firstItem();

		if (!nodeJumper.isNull())
		{
			//get its position
			QRect rec = nodeJumper.getRect();
			if (rec.right() > maxX)
				maxX = rec.right();
			if (rec.bottom() > maxY)
				maxY = rec.bottom();
		}

		//adjust capture size so we don't try to get pixels from outside the document
		if (xOrig > maxX)
			xRemain = xOrig = maxX;
		if (yRemain > maxY)
			yRemain = maxY;
	}

	//scroll to the beginning of the next part
	m_html->view()->verticalScrollBar()->setValue(yPos);
	m_html->view()->horizontalScrollBar()->setValue(xPos);
	kapp->processOneEvent();

	//grab the visible window
	pix = grabChildWidgets(m_html->view()->clipper());

	//get the size of the captured area
	yVisible=pix.height();
	xVisible=pix.width();

	if ((yVisible > yCapture) || (xVisible > xCapture))
	{
		int scrollerVPos=m_html->view()->verticalScrollBar()->value(), //top left position of the visible page part
			scrollerHPos=m_html->view()->horizontalScrollBar()->value(),
			xSize,ySize;
		
		//check if the width of the screenshot should be smaller than the screen width
		if (xVisible > xCapture)
			xSize = xCapture;
		else
			xSize = m_html->view()->clipper()->width();

		//check if the height of the screenshot should be smaller than the screen height
		if (yVisible > yCapture)
			ySize = yCapture;
		else
			ySize = m_html->view()->clipper()->height();

		//create a temporary pixmap for storing the clipped visible window part
		QPixmap temp(xSize, ySize);

		//copy the missing parts from the window in the pixmap
		::bitBlt( &temp, 0, 0, &pix,  xPos-scrollerHPos, yPos-scrollerVPos, xSize, ySize, Qt::CopyROP);
		pix=temp;
	};

	return new QImage(pix.convertToImage());
}





/**
**name timerEvent(QTimerEvent *)
**description Closes the window if capturing is completed.
**/
void KHTML2PNG::timerEvent(QTimerEvent *)
{
	if (!m_completed) {
		m_html->closeURL();
		m_completed = true;
	} else {
		m_flashStarted = true;
	}
}





/**
**name options
**description Array with command line options and descriptions
**/
static KCmdLineOptions options[] =
{
	{ "width ", "Width of canvas on which to render html", "800" },
	{ "height ", "Height of canvas on which to render html", "1000" },
	/*{ "flash-delay ", "Pause after loading page to allow flash movie to fade up", "3" },*/
	{ "time ", "Maximum time in seconds to spend loading page", "30" },
	{ "auto ", "Use this option if you to autodetect the bottom/right border", "" },
	{ "+url ", "URL of page to render", 0 },
	{ "+outfile ", "Output file", 0 },
	{ 0, 0, 0 }
};






int main(int argc, char **argv)
{
	KAboutData aboutData("khtml2png", I18N_NOOP("KHTML2PNG"),
		"2.5.0",
		I18N_NOOP("Render HTML to a PNG from the command line\n\
Example: khtml2png2 --width 800 --height 1000 http://www.kde.org/ kde-org.png\n\
or\n\
khtml2png --auto ID_border http://www.kde.org/ kde-org.png\n\
CAUTION: needs \"convert\" from the imagemagick tools to work properly!"),
		KAboutData::License_GPL,
		"(c) 2003 Simon MacMullen & Hauke Goos-Habermann 2004-2006");
	aboutData.addAuthor("Simon MacMullen", 0, "s.macmullen@ry.com");
	aboutData.addAuthor("Hauke Goos-Habermann", 0, "hhabermann@pc-kiel.de","http://khtml2png.sourceforge.net");
	KCmdLineArgs::init(argc, argv, &aboutData);
	KCmdLineArgs::addCmdLineOptions(options);
	KCmdLineArgs *args=KCmdLineArgs::parsedArgs();

	if (args->count() < 2)
		{
			args->usage();
			exit(1);
		};
	QString temp;
	
	//check for auto detection parameter
	temp = QString::fromLatin1(args->getOption("auto"));
	if (!temp.isEmpty())
		{
			autoDetect = true;
			autoDetectItemName = temp;
		}
	else
		autoDetect = false;
	
	//check for width and height parameters
	if (!autoDetect)
	{
		temp = QString::fromLatin1(args->getOption("width"));
		if (!temp.isEmpty())
			xOrig = temp.toInt();
		else
			{
				fprintf(stderr,"ERROR: You need to set the capture width if you don't use the autodetect mode.\n");
				args->usage();
				exit(2);
			};

		temp = QString::fromLatin1(args->getOption("height"));
		if (!temp.isEmpty())
			yRemain = temp.toInt();
		else
			{
				fprintf(stderr,"ERROR: You need to set the capture height if you don't use the autodetect mode.\n");
				args->usage();
				exit(3);
			};

		detectionCompleted=true;
	};

	int	xNr = 0,
		yNr = 0,
		origWidth;

	//int flashDelay = 2;

	char 	nrStr[10], //temporary buffer to convert int -> char
			cmd[5000], //command line string
			tempName[100]; //the name of the part image file

	/*if (args->isSet("flash-delay"))
	{
		flashDelay = QString::fromLatin1(args->getOption("flash-delay")).toInt();
	}*/

	KInstance inst(&aboutData);
	KApplication app;

	QString path=QString(args->arg(0));

	KHTML2PNG* convertor = new KHTML2PNG();

	//set to 0 so you can see if it has been created
	m_html = 0;

	if (autoDetect)
		convertor->showMiniBrowser(path);

	system("rm /tmp/khtml2png.png_*");
	
	origWidth = xOrig;

	while (!lastXShot || !lastYShot)
		{
			xPos = 0;
			
			lastXShot = false;
			xRemain = xOrig;

			xNr = 0;

			while (!lastXShot)
				{
					//set capture size in the inner loop because xCapture and yCapture are overwritten by convertor->create
					yCapture = yRemain;
					xCapture = xRemain;

					//capture the part of the screen
					QImage* renderedImage = convertor->create(path/*, flashDelay*/);

					//generate the filename of the capture part file
					QString filename = "/tmp/khtml2png.png";
					int g = sprintf(nrStr,"_x%iy%i",xNr,yNr);
					nrStr[g]='\0';
					filename+=QString(nrStr);

					//save the part
					renderedImage->save(filename, "PNG");

					xNr ++;
					xPos += xVisible;
					xRemain -= xVisible;

					if (xRemain <= 0)
						lastXShot=true;
		
					delete renderedImage;
				}

			yPos += yVisible;
			yRemain -= yVisible;
			
			if (yRemain <= 0)
				lastYShot=true;
			yNr++;
		};

	delete(m_html);
	delete(convertor);
	

	//combines the screenshot parts to rows (make a picture of all images in the same row)
	for (int y=0; y < yNr; y++)
		{
			memset(cmd,0,sizeof(cmd));

			//generate convert script to append all parts to one image
			strcpy(cmd,"convert ");

			for (int x=0; x < xNr; x++)
				{
					sprintf(tempName," /tmp/khtml2png.png_x%iy%i" ,x,y);
					strcat(cmd,tempName);
				};

			sprintf(cmd,"%s +append /tmp/khtml2png.png_r%i",cmd,y);
			system(cmd);
			
		}

	memset(cmd,0,sizeof(cmd));

	//append the row images from top to bottom to one image
	strcpy(cmd,"convert ");

	for (int y=0; y < yNr; y++)
		sprintf(cmd,"%s /tmp/khtml2png.png_r%i",cmd,y);
		
	sprintf(cmd,"%s -append %s",cmd,args->arg(1));

	system(cmd);
	exit(0);
}

#include "khtml2png.moc"
