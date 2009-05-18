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

#include "khtml2png.h"

KHTML2PNG::KHTML2PNG()
    : m_html(0)
{
}

KHTML2PNG::~KHTML2PNG()
{
    delete m_html;
}

QImage* KHTML2PNG::create(const QString &path, int width, int height, int time, int flashDelay)
{
    if (!m_html)
    {
        m_html = new KHTMLPart;
        connect(m_html, SIGNAL(completed()), SLOT(slotCompleted()));
        m_html->setJScriptEnabled(true);
        m_html->setJavaEnabled(true);
        m_html->setPluginsEnabled(true);
        m_html->setMetaRefreshEnabled(true);
        m_html->setOnlyLocalReferences(false);
    }
    m_html->openURL(path);
    m_html->view()->resize(width, height);

	if (flashDelay != -1) {
		// Grab using X - requires window to be visible.
		m_html->view()->show();
	}

    m_completed = false;
    startTimer(time * 1000);
    while (!m_completed)
        kapp->processOneEvent();
    killTimers();

    // Do a screengrab type thing - we get form widgets that way

    QPixmap pix;
    if (flashDelay == -1) {
        // Grab using Qt - doesn't work if window contains non-qt widgets
        pix = QPixmap::grabWidget(m_html->view()->clipper());
    } else {
        // Wait for the flash movie to play
        m_flashStarted = false;
        startTimer(flashDelay * 1000);
        while (!m_flashStarted)
            kapp->processOneEvent();
        killTimers();

        pix = QPixmap::grabWindow(m_html->view()->clipper()->winId());
    }

    return new QImage(pix.convertToImage());
}

void KHTML2PNG::timerEvent(QTimerEvent *)
{
    if (!m_completed) {
        m_html->closeURL();
        m_completed = true;
    } else {
        m_flashStarted = true;
    }
}

void KHTML2PNG::slotCompleted()
{
    m_completed = true;
}

static KCmdLineOptions options[] =
{
	{ "width ", "Width of canvas on which to render html", "800" },
	{ "height ", "Height of canvas on which to render html", "1000" },
	{ "scaled-width ", "Width of image to produce", "80" },
	{ "scaled-height ", "Height of image to produce", "100" },
	{ "flash-delay ", "Pause after loading page to allow flash movie to fade up", "3" },
    { "time ", "Maximum time in seconds to spend loading page", "30" },
	{ "+url ", "URL of page to render", 0 },
	{ "+outfile ", "Output file", 0 },
	{ 0, 0, 0 }
};

int main(int argc, char **argv)
{
	KAboutData aboutData("khtml2png", I18N_NOOP("KHTML2PNG"),
		"1.0.2",
		I18N_NOOP("Render HTML to a PNG from the command line\nExample: khtml2png --width 800 --height 1000 --scaled-width 800 --scaled-height 1000 http://www.kde.org/ kde-org.png"),
		KAboutData::License_GPL,
		"(c) 2003 Simon MacMullen.");
	aboutData.addAuthor("Simon MacMullen", 0, "s.macmullen@ry.com");
	KCmdLineArgs::init(argc, argv, &aboutData);
	KCmdLineArgs::addCmdLineOptions(options);
	KCmdLineArgs *args=KCmdLineArgs::parsedArgs();

	int width = QString::fromLatin1(args->getOption("width")).toInt();
	int height = QString::fromLatin1(args->getOption("height")).toInt();
	int scaledWidth = QString::fromLatin1(args->getOption("scaled-width")).toInt();
	int scaledHeight = QString::fromLatin1(args->getOption("scaled-height")).toInt();
	int time = QString::fromLatin1(args->getOption("time")).toInt();

    int flashDelay = -1;

    if (args->isSet("flash-delay")){
        flashDelay = QString::fromLatin1(args->getOption("flash-delay")).toInt();
    }

	KInstance inst(&aboutData);
	KApplication app;

	if (args->count() != 2) {
	    args->usage();
	}

	KHTML2PNG* convertor = new KHTML2PNG();
	QImage* renderedImage = convertor->create(QString(args->arg(0)), width, height, time, flashDelay);

	QString filename = QString(args->arg(1));

	if (renderedImage->width() < scaledWidth || renderedImage->height() < scaledHeight) {
		// We got a slightly smaller image than we asked for (scrollbars)
		// don't blow it up by a few % (looks blurry)
		if (filename.endsWith(".jpeg") || filename.endsWith(".jpg")) {
			renderedImage->save(args->arg(1), "JPEG");
		} else {
			renderedImage->save(args->arg(1), "PNG");
		}

	} else {
		QImage scaledImage = renderedImage->smoothScale(scaledWidth, scaledHeight);

		if (filename.endsWith(".jpeg") || filename.endsWith(".jpg")) {
			scaledImage.save(args->arg(1), "JPEG");
		} else {
			scaledImage.save(args->arg(1), "PNG");
		}
	}

	delete renderedImage;
	delete convertor;
}

#include "khtml2png.moc"
