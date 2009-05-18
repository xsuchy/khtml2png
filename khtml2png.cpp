/*  Render HTML page, write out as PNG
    Heavily based on KDE HTML thumbnail creator
    Copyright (C) 2003 Simon MacMullen
    Copyright (C) 2004 Miroslav Suchy
    Copyright (C) 2009 Dilshod Mukhtarov

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

#include <KApplication>
#include <khtml_part.h>
#include <KHTMLView>
#include <KCmdLineArgs>
#include <KLocale>
#include <KAboutData>
#include <iostream>
#include <string.h>
#include <dom/dom_core.h>
#include <dom/html_base.h>
#include <dom/dom_string.h>
#include <kio/job.h>


#include "khtml2png.h"

KHTML2PNG::KHTML2PNG(int jscript, int java, int plugin, int refresh, int localonly)
    : m_html(0) {
	m_html = new myKHTMLPart;
        connect(m_html, SIGNAL(completed()), SLOT(slotCompleted()));
        m_html->setJScriptEnabled(true);
        m_html->setJavaEnabled(true);
        m_html->setPluginsEnabled(true);
        m_html->setMetaRefreshEnabled(true);
        m_html->setOnlyLocalReferences(false);
}

QImage* KHTML2PNG::create(const QString &path, int width, int height, int time, int flashDelay) {
    m_html->openUrl(path);
    m_html->view()->resize(width, height);

    m_completed = false;
    int timerId = startTimer(time * 1000);
    while (!m_completed) {
		usleep(1);
        	kapp->processEvents();
	}
	killTimer(timerId);
	flashflag = false;
   	showTree(m_html->document());
    // Do a screengrab type thing - we get form widgets that way

	QPixmap pix;
	if ((flashDelay > 0) || (flashflag)) { // Grab using X - requires window to be visible.
		m_html->view()->show();
		// Wait for the flash movie to play
	        m_flashStarted = false;
        	timerId = startTimer(flashDelay > 0 ? flashDelay * 1000 : 3000);
        	while (!m_flashStarted) {
			usleep(1);
        		kapp->processEvents();
		}
        	killTimer(timerId);
	        pix = QPixmap::grabWindow(m_html->view()->viewport()->winId());
	} else {
		m_html->view()->hide();
		// Grab using Qt - doesn't work if window contains non-qt widgets
        	pix = QPixmap::grabWidget(m_html->view()->viewport());
	}

    QImage *image = new QImage(pix.toImage());
    // do not spend time with active items, render blank page and wait for others
    m_html->closeUrl();

    return image;
}

QString KHTML2PNG::lastModified() {
	return m_html->lastModified();
}

bool KHTML2PNG::haveFlash() {
	return flashflag;
}

void KHTML2PNG::timerEvent(QTimerEvent *) {
    if (!m_completed) {
         m_completed = true;
    } else {
        m_flashStarted = true;
    }
}

void KHTML2PNG::slotCompleted()
{
    m_completed = true;
	printf("completed()\n");
}

void KHTML2PNG::showTree(const DOM::Node &pNode) {
  DOM::Node child;

  if (!pNode.isNull()) { //page is not loaded, probably due to transfer error
  try {
    child = pNode.firstChild();
  }
  catch (DOM::DOMException &)   {
    return;
  }

  while(!child.isNull() && !flashflag) {
    showRecursive(child);
    child = child.nextSibling();
  }
  }
}

void KHTML2PNG::showRecursive(const DOM::Node &node) {
  if(node.nodeName().string() == QString::fromLatin1("OBJECT")) {
    flashflag = true; // detected
    return;
  }
  DOM::Node child = node.lastChild();
  if (child.isNull()) {
    DOM::HTMLFrameElement frame = node;
    if(!frame.isNull()) 
	child = frame.contentDocument().documentElement();
  }
  while(!child.isNull() && !flashflag) {
    showRecursive(child);
    child = child.previousSibling();
  }
}

void KHTML2PNG::processBatch(const QString &fileName, const QString &path, int width, int height, int time, int flashDelay, int scaledWidth, int scaledHeight) {
	QImage* renderedImage = this->create(path, width, height, time, flashDelay);

	if (renderedImage->width() < scaledWidth || renderedImage->height() < scaledHeight) {
		// We got a slightly smaller image than we asked for (scrollbars)
		// don't blow it up by a few % (looks blurry)
		if (fileName.endsWith(".jpeg") || fileName.endsWith(".jpg")) {
			renderedImage->save(fileName, "JPEG");
		} else {
			renderedImage->save(fileName, "PNG");
		}

	} else {
		QImage scaledImage = renderedImage->scaled(scaledWidth, scaledHeight);

		if (fileName.endsWith(".jpeg") || fileName.endsWith(".jpg")) {
			scaledImage.save(fileName, "JPEG");
		} else {
			scaledImage.save(fileName, "PNG");
		}
	}
	delete renderedImage;
}

int main(int argc, char **argv) {
	KCmdLineOptions options;
	options.add( "width ", ki18n("Width of canvas on which to render html"), "800" );
	options.add( "height ", ki18n("Height of canvas on which to render html"), "1000" );
	options.add( "scaled-width ", ki18n("Width of image to produce"), "80" );
	options.add( "scaled-height ", ki18n("Height of image to produce"), "100" );
	options.add( "flash-delay ", ki18n("Pause after loading page to allow flash movie to fade up (-1 mean no flash)"), "3" );
	options.add( "time ", ki18n("Maximum time in seconds to spend loading page"), "30" );
	options.add( "nojscript", ki18n("Disable JavaScript") );
	options.add( "nojava", ki18n("Disable Java") );
	options.add( "noplugin", ki18n("Disable plugins") );
	options.add( "norefresh", ki18n("Disable meta refresh") );
	options.add( "localonly", ki18n("Disable local references") );
	options.add( "+url ", ki18n("URL of page to render") );
	options.add( "+outfile ", ki18n("Output file") );

	KAboutData aboutData("khtml2png", "KHTML2PNG", ki18n("khtml2png"),
		"1.0.3",
		ki18n("Render HTML to a PNG from the command line\nExample: khtml2png --width 800 --height 1000 --scaled-width 800 --scaled-height 1000 http://www.kde.org/ kde-org.png\nor khtml2png --width 800 --height 1000 - and write url on first line of stdin and on second 'filename scaledwidth scaledheight flash'"),
		KAboutData::License_GPL,
		ki18n("(c) 2003 Simon MacMullen."));
	aboutData.addAuthor(ki18n("Simon MacMullen"), ki18n(0), "s.macmullen@ry.com");
	aboutData.addAuthor(ki18n("Miroslav Suchy"), ki18n(0), "miroslav@suchy.cz");
        aboutData.addAuthor(ki18n("Dilshod Mukhtarov"), ki18n(0), "dilshodm at gmail dot com");
	KCmdLineArgs::init(argc, argv, &aboutData);
	KCmdLineArgs::addCmdLineOptions(options);
	KCmdLineArgs *args=KCmdLineArgs::parsedArgs();

	int width = args->getOption("width").toInt();
	int height = args->getOption("height").toInt();
	int scaledWidth = args->getOption("scaled-width").toInt();
	int scaledHeight = args->getOption("scaled-height").toInt();
	int time = args->getOption("time").toInt();

    int flashDelay = -1;

    if (args->isSet("flash-delay")){
        flashDelay = args->getOption("flash-delay").toInt();
    }

	KApplication app;
	char Input [2000];
	char ids [2000] = "-";
	if (args->count() != 2) {
		if (args->count() == 0 || strcmp(args->arg(0).toAscii().data(),ids)!=0)  {
		    args->usage();
		}
	}
	KHTML2PNG* convertor = new KHTML2PNG(args->isSet("jscript"),
			args->isSet("java"),
			args->isSet("plugin"),
			args->isSet("refresh"),
			args->isSet("localonly"));
    
	if (strcmp(args->arg(0).toAscii().data(),ids)==0) {
		while (scanf(" %[^\n]",  Input) != EOF && scanf(" %s", ids) != EOF 
			&& scanf(" %d", &scaledWidth) != EOF && scanf(" %d", &scaledHeight)
			&& scanf(" %d", &flashDelay)) {
			convertor->processBatch(ids, Input, width, height, time, flashDelay, scaledWidth, scaledHeight);
			printf ("Thumbnail from %s saved to %s (flash: %s; last modified: %s).\n", Input, ids, convertor->haveFlash()?"yes":"no", convertor->lastModified().isNull()?"":convertor->lastModified().toAscii().data());
			fflush(NULL);
		}
	} else {
		convertor->processBatch(args->arg(1), args->arg(0), width, height, time, flashDelay, scaledWidth, scaledHeight);	
	}
	delete convertor;
}

#include "khtml2png.moc"
