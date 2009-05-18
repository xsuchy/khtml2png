/*  Render HTML page, write out as PNG
	Heavily based on KDE HTML thumbnail creator
	Copyright (C) 2003 Simon MacMullen

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

#include <kapplication.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <stdlib.h>
#include <kde/dom/html_misc.h>

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
	m_html->setMetaRefreshEnabled(true);
	m_html->setOnlyLocalReferences(false);		
	m_html->view()->setResizePolicy(QScrollView::Manual);

	//this is needed for navigation on the page e.g. clicking on links
	connect( m_html->browserExtension(),
	SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
	this, SLOT( openURLRequest(const KURL &, const KParts::URLArgs & ) ) );

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
QImage* KHTML2PNG::create(const QString &path, int flashDelay)
{
	if (!m_html)
	{
		/*
			if the KHTMLPart object doesn't exist, create it
			set some basic HTML settings
			and open the window
		*/
		create_m_html();
	}

	//open the HTMl page in the window
	m_html->openURL(path);
	kapp->processOneEvent();
	
	m_html->view()->setMarginWidth(0);
	m_html->view()->setMarginHeight(0);

	//resize it to the wanted size
	m_html->view()->resize(xCapture, yCapture);
	kapp->processOneEvent();
	

	m_completed = false;
	//start loop
	while (!m_completed)
		kapp->processOneEvent();
	killTimers();

	//scroll to the beginning of the next part
	m_html->view()->verticalScrollBar()->setValue(yPos);
	m_html->view()->horizontalScrollBar()->setValue(xPos);
	kapp->processOneEvent();

	QPixmap pix;

	// Wait for the flash movie to play
	m_flashStarted = false;
	startTimer(flashDelay * 1000);
		while (!m_flashStarted)
		kapp->processOneEvent();
	killTimers();

    //grab the window
	pix = QPixmap::grabWindow(m_html->view()->clipper()->winId());	

	//get the size of the captured aread
	yVisible=pix.height();
	xVisible=pix.width();

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
	{ "flash-delay ", "Pause after loading page to allow flash movie to fade up", "3" },
	{ "time ", "Maximum time in seconds to spend loading page", "30" },
	{ "auto ", "Use this option if you to autodetect the bottom/right border", "" },
	{ "+url ", "URL of page to render", 0 },
	{ "+outfile ", "Output file", 0 },
	{ 0, 0, 0 }
};






int main(int argc, char **argv)
{
	KAboutData aboutData("khtml2png", I18N_NOOP("KHTML2PNG"),
		"2.0.1",
		I18N_NOOP("Render HTML to a PNG from the command line\n\
Example: khtml2png --width 800 --height 1000 http://www.kde.org/ kde-org.png\n\
or\n\
khtml2png --auto ID_border http://www.kde.org/ kde-org.png\n\
CAUTION: needs \"convert\" from the imagemagick tools to work properly!"),
		KAboutData::License_GPL,
		"(c) 2003 Simon MacMullen & Hauke Goos-Habermann 2004-2005");
	aboutData.addAuthor("Simon MacMullen", 0, "s.macmullen@ry.com");
	aboutData.addAuthor("Hauke Goos-Habermann", 0, "hhabermann@pc-kiel.de","http://m23.sf.net");
	KCmdLineArgs::init(argc, argv, &aboutData);
	KCmdLineArgs::addCmdLineOptions(options);
	KCmdLineArgs *args=KCmdLineArgs::parsedArgs();

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
		yNr = 0;

	int flashDelay = 2;

	char 	nrStr[5], //temporary buffer to convert int -> char
			cmd[5000], //command line string
			tempName[100]; //the name of the part image file

	if (args->isSet("flash-delay"))
	{
		flashDelay = QString::fromLatin1(args->getOption("flash-delay")).toInt();
	}

	KInstance inst(&aboutData);
	KApplication app;

	QString path=QString(args->arg(0));

	KHTML2PNG* convertor = new KHTML2PNG();

	//set to 0 so you can see if it has been created
	m_html = 0;

	if (autoDetect)
		convertor->showMiniBrowser(path);

	while (!lastXShot || !lastYShot)
		{
			xPos = 0;
			
			lastXShot = false;
			xRemain = xOrig;

			xNr = 0;

			yCapture = yRemain + 10;

			while (!lastXShot)
				{
					xCapture = xRemain + 18;

					//capture the part of the screen
					QImage* renderedImage = convertor->create(path, flashDelay);

					//generate the filename of the capture part file
					QString filename = "/tmp/khtml2png.png";
					sprintf(nrStr,"_x%iy%i",xNr,yNr);
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

	delete convertor;
}

#include "khtml2png.moc"
