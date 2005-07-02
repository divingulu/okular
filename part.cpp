/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2002 by Chris Cheney <ccheney@cheney.cx>                *
 *   Copyright (C) 2002 by Malcolm Hunter <malcolm.hunter@gmx.co.uk>       *
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Daniel Molkentin <molkentin@kde.org>            *
 *   Copyright (C) 2003 by Andy Goossens <andygoossens@telenet.be>         *
 *   Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                  *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Christoph Cullmann <crossfire@babylon2k.de>     *
 *   Copyright (C) 2004 by Henrique Pinto <stampede@coltec.ufmg.br>        *
 *   Copyright (C) 2004 by Waldo Bastian <bastian@kde.org>                 *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *   Copyright (C) 2004 by Antti Markus <antti.markus@starman.ee>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qsplitter.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qtoolbox.h>
#include <qpushbutton.h>
#include <dcopobject.h>
#include <dcopclient.h>
#include <kapplication.h>
#include <kaction.h>
#include <kdirwatch.h>
#include <kinstance.h>
#include <kprinter.h>
#include <kstdaction.h>
#include <kdeversion.h>
#include <kparts/genericfactory.h>
#include <kurldrag.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kfinddialog.h>
#include <knuminput.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kpopupmenu.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <ktrader.h>
#include <kxmlguiclient.h>
#include <kxmlguifactory.h>

// local includes
#include "xpdf/GlobalParams.h"
#include "part.h"
#include "ui/pageview.h"
#include "ui/thumbnaillist.h"
#include "ui/searchwidget.h"
#include "ui/toc.h"
#include "ui/minibar.h"
#include "ui/propertiesdialog.h"
#include "ui/presentationwidget.h"
#include "conf/preferencesdialog.h"
#include "conf/settings.h"
#include "core/document.h"
#include "core/page.h"

// definition of searchID for this class
#define PART_SEARCH_ID 1

typedef KParts::GenericFactory<KPDF::Part> KPDFPartFactory;
K_EXPORT_COMPONENT_FACTORY(libkpdfpart, KPDFPartFactory)

using namespace KPDF;

unsigned int Part::m_count = 0;

Part::Part(QWidget *parentWidget, const char *widgetName,
           QObject *parent, const char *name,
           const QStringList & /*args*/ )
	: DCOPObject("kpdf"), KParts::ReadOnlyPart(parent, name), m_showMenuBarAction(0), m_showFullScreenAction(0),
	m_actionsSearched(false), m_searchStarted(false)
{
	// load catalog for translation
	KGlobal::locale()->insertCatalogue("kpdf");

	// create browser extension (for printing when embedded into browser)
	m_bExtension = new BrowserExtension(this);

	// xpdf 'extern' global class (m_count is a static instance counter)
	//if ( m_count ) TODO check if we need to insert these lines..
	//	delete globalParams;
	globalParams = new GlobalParams("");
	globalParams->setupBaseFonts(NULL);
	m_count++;

	// we need an instance
	setInstance(KPDFPartFactory::instance());

	// build the document
	m_document = new KPDFDocument();
	connect( m_document, SIGNAL( linkFind() ), this, SLOT( slotFind() ) );
	connect( m_document, SIGNAL( linkGoToPage() ), this, SLOT( slotGoToPage() ) );
	connect( m_document, SIGNAL( openURL(const KURL &) ), this, SLOT( openURL(const KURL &) ) );

	// widgets: ^searchbar (toolbar containing label and SearchWidget)
//	m_searchToolBar = new KToolBar( parentWidget, "searchBar" );
//	m_searchToolBar->boxLayout()->setSpacing( KDialog::spacingHint() );
//	QLabel * sLabel = new QLabel( i18n( "&Search:" ), m_searchToolBar, "kde toolbar widget" );
//	m_searchWidget = new SearchWidget( m_searchToolBar, m_document );
//	sLabel->setBuddy( m_searchWidget );
//	m_searchToolBar->setStretchableWidget( m_searchWidget );

	// widgets: [] splitter []
	m_splitter = new QSplitter( parentWidget, widgetName );
	m_splitter->setOpaqueResize( true );
	setWidget( m_splitter );
	
	m_showLeftPanel = new KToggleAction( i18n( "Show &Navigation Panel"), 0, this, SLOT( slotShowLeftPanel() ), actionCollection(), "show_leftpanel" );
	m_showLeftPanel->setCheckedState( i18n( "Hide &Navigation Panel") );
	m_showLeftPanel->setShortcut( "CTRL+L" );
	m_showLeftPanel->setChecked( Settings::showLeftPanel() );

	// widgets: [left panel] | []
	m_leftPanel = new QWidget( m_splitter );
	m_leftPanel->setMinimumWidth( 90 );
	m_leftPanel->setMaximumWidth( 300 );
	QVBoxLayout * leftPanelLayout = new QVBoxLayout( m_leftPanel );

	// widgets: [left toolbox/..] | []
	m_toolBox = new QToolBox( m_leftPanel );
	leftPanelLayout->addWidget( m_toolBox );

	int index;
	// [left toolbox: Table of Contents] | []
	TOC * tocFrame = new TOC( m_toolBox, m_document );
	connect(tocFrame, SIGNAL(hasTOC(bool)), this, SLOT(enableTOC(bool)));
	index = m_toolBox->addItem( tocFrame, QIconSet(SmallIcon("text_left")), i18n("Contents") );
	m_toolBox->setItemToolTip(index, i18n("Contents"));
	enableTOC( false );

	// [left toolbox: Thumbnails and Bookmarks] | []
	QVBox * thumbsBox = new ThumbnailsBox( m_toolBox );
	m_searchWidget = new SearchWidget( thumbsBox, m_document );
	m_thumbnailList = new ThumbnailList( thumbsBox, m_document );
//	ThumbnailController * m_tc = new ThumbnailController( thumbsBox, m_thumbnailList );
	connect( m_thumbnailList, SIGNAL( urlDropped( const KURL& ) ), SLOT( openURL( const KURL & )) );
	connect( m_thumbnailList, SIGNAL( rightClick(const KPDFPage *, const QPoint &) ), this, SLOT( slotShowMenu(const KPDFPage *, const QPoint &) ) );
	// shrink the bottom controller toolbar (too hackish..)
	thumbsBox->setStretchFactor( m_searchWidget, 100 );
	thumbsBox->setStretchFactor( m_thumbnailList, 100 );
//	thumbsBox->setStretchFactor( m_tc, 1 );
	index = m_toolBox->addItem( thumbsBox, QIconSet(SmallIcon("thumbnail")), i18n("Thumbnails") );
	m_toolBox->setItemToolTip(index, i18n("Thumbnails"));
	m_toolBox->setCurrentItem( thumbsBox );

	slotShowLeftPanel();

/*	// [left toolbox: Annotations] | []
	QFrame * editFrame = new QFrame( m_toolBox );
	int iIdx = m_toolBox->addItem( editFrame, QIconSet(SmallIcon("pencil")), i18n("Annotations") );
	m_toolBox->setItemEnabled( iIdx, false );*/

	// widgets: [../miniBarContainer] | []
	QWidget * miniBarContainer = new QWidget( m_leftPanel );
	leftPanelLayout->addWidget( miniBarContainer );
	QVBoxLayout * miniBarLayout = new QVBoxLayout( miniBarContainer );
	// widgets: [../[spacer/..]] | []
	QWidget * miniSpacer = new QWidget( miniBarContainer );
	miniSpacer->setFixedHeight( 6 );
	miniBarLayout->addWidget( miniSpacer );
	// widgets: [../[../MiniBar]] | []
	MiniBar * miniBar = new MiniBar( miniBarContainer, m_document );
	miniBarLayout->addWidget( miniBar );

	// widgets: [] | [right 'pageView']
	m_pageView = new PageView( m_splitter, m_document );
	m_pageView->setFocus(); //usability setting
	connect( m_pageView, SIGNAL( urlDropped( const KURL& ) ), SLOT( openURL( const KURL & )));
	connect( m_pageView, SIGNAL( rightClick(const KPDFPage *, const QPoint &) ), this, SLOT( slotShowMenu(const KPDFPage *, const QPoint &) ) );

	// add document observers
	m_document->addObserver( this );
	m_document->addObserver( m_thumbnailList );
	m_document->addObserver( m_pageView );
	m_document->addObserver( tocFrame );
	m_document->addObserver( miniBar );

	// ACTIONS
	KActionCollection * ac = actionCollection();

	// Page Traversal actions
	m_gotoPage = KStdAction::gotoPage( this, SLOT( slotGoToPage() ), ac, "goto_page" );
	m_gotoPage->setShortcut( "CTRL+G" );
	// dirty way to activate gotopage when pressing miniBar's button
	connect( miniBar, SIGNAL( gotoPage() ), m_gotoPage, SLOT( activate() ) );

	m_prevPage = KStdAction::prior(this, SLOT(slotPreviousPage()), ac, "previous_page");
	m_prevPage->setWhatsThis( i18n( "Moves to the previous page of the document" ) );
	m_prevPage->setShortcut( "Backspace" );
	// dirty way to activate prev page when pressing miniBar's button
	connect( miniBar, SIGNAL( prevPage() ), m_prevPage, SLOT( activate() ) );

	m_nextPage = KStdAction::next(this, SLOT(slotNextPage()), ac, "next_page" );
	m_nextPage->setWhatsThis( i18n( "Moves to the next page of the document" ) );
	m_nextPage->setShortcut( "Space" );
	// dirty way to activate next page when pressing miniBar's button
	connect( miniBar, SIGNAL( nextPage() ), m_nextPage, SLOT( activate() ) );

	m_firstPage = KStdAction::firstPage( this, SLOT( slotGotoFirst() ), ac, "first_page" );
	m_firstPage->setWhatsThis( i18n( "Moves to the first page of the document" ) );

	m_lastPage = KStdAction::lastPage( this, SLOT( slotGotoLast() ), ac, "last_page" );
	m_lastPage->setWhatsThis( i18n( "Moves to the last page of the document" ) );

	m_historyBack = KStdAction::back( this, SLOT( slotHistoryBack() ), ac, "history_back" );
	m_historyBack->setWhatsThis( i18n( "Go to the place you were before" ) );

	m_historyNext = KStdAction::forward( this, SLOT( slotHistoryNext() ), ac, "history_forward" );
	m_historyNext->setWhatsThis( i18n( "Go to the place you were after" ) );

	// Find and other actions
	m_find = KStdAction::find( this, SLOT( slotFind() ), ac, "find" );
	m_find->setEnabled( false );

	m_findNext = KStdAction::findNext( this, SLOT( slotFindNext() ), ac, "find_next" );
	m_findNext->setEnabled( false );

	m_saveAs = KStdAction::saveAs( this, SLOT( slotSaveFileAs() ), ac, "save" );
	m_saveAs->setEnabled( false );
	KAction * prefs = KStdAction::preferences( this, SLOT( slotPreferences() ), ac, "preferences" );
	prefs->setText( i18n( "Configure KPDF..." ) );
	m_printPreview = KStdAction::printPreview( this, SLOT( slotPrintPreview() ), ac );
	m_printPreview->setEnabled( false );

	m_showProperties = new KAction(i18n("&Properties"), "info", 0, this, SLOT(slotShowProperties()), ac, "properties");
	m_showProperties->setEnabled( false );

	m_showPresentation = new KAction( i18n("P&resentation"), "kpresenter_kpr", "Ctrl+Shift+P", this, SLOT(slotShowPresentation()), ac, "presentation");
	m_showPresentation->setEnabled( false );

	// attach the actions of the children widgets too
	m_pageView->setupActions( ac );

	// apply configuration (both internal settings and GUI configured items)
	QValueList<int> splitterSizes = Settings::splitterSizes();
	if ( !splitterSizes.count() )
	{
		// the first time use 1/10 for the panel and 9/10 for the pageView
		splitterSizes.push_back( 50 );
		splitterSizes.push_back( 500 );
	}
	m_splitter->setSizes( splitterSizes );
	m_watcher = new KDirWatch( this );
	connect( m_watcher, SIGNAL( dirty( const QString& ) ), this, SLOT( slotFileDirty( const QString& ) ) );
	m_dirtyHandler = new QTimer( this );
	connect( m_dirtyHandler, SIGNAL( timeout() ),this, SLOT( slotDoFileDirty() ) );

	slotNewConfig();

	// [SPEECH] check for KTTSD presence and usability
	KTrader::OfferList offers = KTrader::self()->query("DCOP/Text-to-Speech", "Name == 'KTTSD'");
	Settings::setUseKTTSD( (offers.count() > 0) );

	// set our XML-UI resource file
	setXMLFile("part.rc");
	updateViewActions();
}

Part::~Part()
{
    // save internal settings
    Settings::setSplitterSizes( m_splitter->sizes() );
    // write to disk config file
    Settings::writeConfig();

    delete m_document;
    if ( --m_count == 0 )
        delete globalParams;
}

void Part::notifyViewportChanged( bool /*smoothMove*/ )
{
    // update actions if the page is changed
    static int lastPage = -1;
    int viewportPage = m_document->viewport().pageNumber;
    if ( viewportPage != lastPage )
    {
        updateViewActions();
        lastPage = viewportPage;
    }
}

void Part::goToPage(uint i)
{
    if ( i <= m_document->pages() )
        m_document->setViewportPage( i - 1 );
}

void Part::openDocument(KURL doc)
{
	openURL(doc);
}

uint Part::pages()
{
	return m_document->pages();
}

uint Part::currentPage()
{
	if ( m_document->pages() == 0 ) return 0;
	else return m_document->currentPage()+1;
}

KURL Part::currentDocument()
{
	return m_document->currentDocument();	
}

//this don't go anywhere but is required by genericfactory.h
KAboutData* Part::createAboutData()
{
	// the non-i18n name here must be the same as the directory in
	// which the part's rc file is installed ('partrcdir' in the
	// Makefile)
	KAboutData* aboutData = new KAboutData("kpdfpart", I18N_NOOP("KPDF::Part"), "0.1");
	aboutData->addAuthor("Wilco Greven", 0, "greven@kde.org");
	return aboutData;
}

bool Part::openFile()
{
    KMimeType::Ptr mime = KMimeType::findByPath( m_file );
    if ( (*mime).is( "application/postscript" ) )
    {
        QString app = KStandardDirs::findExe( "ps2pdf" );
        if ( !app.isNull() )
        {
            KTempFile tf( QString::null, ".pdf" );
            if ( tf.status() == 0 )
            {
                tf.close();
                m_temporaryLocalFile = tf.name();

                KProcess *p = new KProcess;
                *p << app;
                *p << m_file << m_temporaryLocalFile;
                m_pageView->showText(i18n("Transforming from ps to pdf..."), 0);
                connect(p, SIGNAL(processExited(KProcess *)), this, SLOT(psTransformEnded()));
                p -> start();
                return true;
            }
        }
        else
        {
            KMessageBox::error(widget(), i18n("You don't have ps2pdf installed so kpdf can not open postscript files."));
            return false;
        }
    }

    m_temporaryLocalFile = QString::null;

    bool ok = m_document->openDocument( m_file, url() );

    // update one-time actions
    m_find->setEnabled( ok && m_document-> supportsSearching());
    m_findNext->setEnabled( ok && m_document-> supportsSearching());
    m_saveAs->setEnabled( ok );
    m_printPreview->setEnabled( ok );
    m_showProperties->setEnabled( ok );
    m_showPresentation->setEnabled( ok );

    // update viewing actions
    updateViewActions();

    if ( !ok )
    {
        // if can't open document, update windows so they display blank contents
        m_pageView->updateContents();
        m_thumbnailList->updateContents();
        return false;
    }

    // set the file to the fileWatcher
    if ( !m_watcher->contains(m_file) )
        m_watcher->addFile(m_file);

    // if the 'StartFullScreen' flag is set, start presentation
    if ( m_document->getMetaData( "StartFullScreen" ) == "yes" )
    {
	KMessageBox::information(m_presentationWidget, i18n("The document is going to be launched on presentation mode because the file requested it."), QString::null, "autoPresentationWarning");
        slotShowPresentation();
    }

    return true;
}

bool Part::openURL(const KURL &url)
{
    // note: this can be the right place to check the file for gz or bz2 extension
    // if it matches then: download it (if not local) extract to a temp file using
    // KTar and proceed with the URL of the temporary file

    // this calls the above 'openURL' method
    bool b = KParts::ReadOnlyPart::openURL(url);
    m_bExtension->openURLNotify();
    m_bExtension->setLocationBarURL(url.prettyURL());
    if ( !b )
        KMessageBox::error( widget(), i18n("Could not open %1").arg( url.prettyURL() ) );
    else
        m_viewportDirty.pageNumber = -1;
    return b;
}

bool Part::closeURL()
{
    if (!m_temporaryLocalFile.isNull())
    {
        QFile::remove( m_temporaryLocalFile );
        m_temporaryLocalFile = QString::null;
    }

    m_find->setEnabled( false );
    m_findNext->setEnabled( false );
    m_saveAs->setEnabled( false );
    m_printPreview->setEnabled( false );
    m_showProperties->setEnabled( false );
    m_showPresentation->setEnabled( false );
    updateViewActions();
    m_searchStarted = false;
    if (!m_file.isEmpty()) m_watcher->removeFile(m_file);
    m_document->closeDocument();
    m_searchWidget->clearText();
    return KParts::ReadOnlyPart::closeURL();
}

void Part::slotShowLeftPanel()
{
    bool showLeft = m_showLeftPanel->isChecked();
    Settings::setShowLeftPanel(showLeft);
    // show/hide left qtoolbox
    m_leftPanel->setShown( showLeft );
    // this needs to be hidden explicitly to disable thumbnails gen
    m_thumbnailList->setShown( showLeft );
}

void Part::slotFileDirty( const QString& fileName )
{
  // The beauty of this is that each start cancels the previous one.
  // This means that timeout() is only fired when there have
  // no changes to the file for the last 750 milisecs.
  // This is supposed to ensure that we don't update on every other byte
  // that gets written to the file.
  if ( fileName == m_file )
  {
    m_dirtyHandler->start( 750, true );
  }
}

void Part::slotDoFileDirty()
{
  if (m_viewportDirty.pageNumber == -1)
  {
    m_viewportDirty = m_document->viewport();
    m_pageView->showText(i18n("Reloading the document..."), 0);
  }

  if (KParts::ReadOnlyPart::openURL(m_file))
  {
    if (m_viewportDirty.pageNumber >= (int)m_document->pages()) m_viewportDirty.pageNumber = (int)m_document->pages() - 1;
    m_document->setViewport(m_viewportDirty);
    m_viewportDirty.pageNumber = -1;
  }
  else
  {
    m_watcher->addFile(m_file);
    m_dirtyHandler->start( 750, true );
  }
}

void Part::updateViewActions()
{
    bool opened = m_document->pages() > 0;
    if ( opened )
    {
        bool atBegin = m_document->currentPage() < 1;
        bool atEnd = m_document->currentPage() >= (m_document->pages() - 1);
        m_gotoPage->setEnabled( m_document->pages() > 1 );
        m_firstPage->setEnabled( !atBegin );
        m_prevPage->setEnabled( !atBegin );
        m_lastPage->setEnabled( !atEnd );
        m_nextPage->setEnabled( !atEnd );
        m_historyBack->setEnabled( !m_document->historyAtBegin() );
        m_historyNext->setEnabled( !m_document->historyAtEnd() );
    }
    else
    {
        m_gotoPage->setEnabled( false );
        m_firstPage->setEnabled( false );
        m_lastPage->setEnabled( false );
        m_prevPage->setEnabled( false );
        m_nextPage->setEnabled( false );
        m_historyBack->setEnabled( false );
        m_historyNext->setEnabled( false );
    }
}

void Part::enableTOC(bool enable)
{
	m_toolBox->setItemEnabled(0, enable);
}

void Part::psTransformEnded()
{
	m_file = m_temporaryLocalFile;
	openFile();
}

//BEGIN go to page dialog
class KPDFGotoPageDialog : public KDialogBase
{
public:
	KPDFGotoPageDialog(QWidget *p, int current, int max) : KDialogBase(p, 0L, true, i18n("Go to Page"), Ok | Cancel, Ok) {
		QWidget *w = new QWidget(this);
		setMainWidget(w);

		QVBoxLayout *topLayout = new QVBoxLayout( w, 0, spacingHint() );
		e1 = new KIntNumInput(current, w);
		e1->setRange(1, max);
		e1->setEditFocus(true);

		QLabel *label = new QLabel( e1,i18n("&Page:"), w );
		topLayout->addWidget(label);
		topLayout->addWidget(e1);
		topLayout->addSpacing(spacingHint()); // A little bit extra space
		topLayout->addStretch(10);
		e1->setFocus();
	}

	int getPage() {
		return e1->value();
	}

  protected:
    KIntNumInput *e1;
};
//END go to page dialog

void Part::slotGoToPage()
{
    KPDFGotoPageDialog pageDialog( m_pageView, m_document->currentPage() + 1, m_document->pages() );
    if ( pageDialog.exec() == QDialog::Accepted )
        m_document->setViewportPage( pageDialog.getPage() - 1 );
}

void Part::slotPreviousPage()
{
    if ( !m_document->currentPage() < 1 )
        m_document->setViewportPage( m_document->currentPage() - 1 );
}

void Part::slotNextPage()
{
    if ( m_document->currentPage() < (m_document->pages() - 1) )
        m_document->setViewportPage( m_document->currentPage() + 1 );
}

void Part::slotGotoFirst()
{
    m_document->setViewportPage( 0 );
}

void Part::slotGotoLast()
{
    m_document->setViewportPage( m_document->pages() - 1 );
}

void Part::slotHistoryBack()
{
    m_document->setPrevViewport();
}

void Part::slotHistoryNext()
{
    m_document->setNextViewport();
}

void Part::slotFind()
{
    KFindDialog dlg( widget() );
    dlg.setHasCursor( false );
#if KDE_IS_VERSION(3,3,90)
    dlg.setSupportsBackwardsFind( false );
    dlg.setSupportsWholeWordsFind( false );
    dlg.setSupportsRegularExpressionFind( false );
#endif
    if ( dlg.exec() == QDialog::Accepted )
    {
        m_searchStarted = true;
        m_document->resetSearch( PART_SEARCH_ID );
        m_document->searchText( PART_SEARCH_ID, dlg.pattern(), false, dlg.options() & KFindDialog::CaseSensitive,
                                KPDFDocument::NextMatch, true, qRgb( 255, 255, 64 ) );
    }
}

void Part::slotFindNext()
{
    if ( m_searchStarted )
        m_document->continueSearch( PART_SEARCH_ID );
    else
        slotFind();
}

void Part::slotSaveFileAs()
{
    KURL saveURL = KFileDialog::getSaveURL( url().isLocalFile() ? url().url() : url().fileName(), QString::null, widget() );
    if ( saveURL.isValid() && !saveURL.isEmpty() )
    {
        if ( KIO::NetAccess::exists( saveURL, false, widget() ) )
        {
            if (KMessageBox::questionYesNo( widget(), i18n("A file named \"%1\" already exists. Are you sure you want to overwrite it?").arg(saveURL.filename())) != KMessageBox::Yes)
                return;
        }

        if ( !KIO::NetAccess::file_copy( url(), saveURL, -1, true ) )
            KMessageBox::information( 0, i18n("File could not be saved in '%1'. Try to save it to another location.").arg( saveURL.prettyURL() ) );
    }
}

void Part::slotPreferences()
{
    // an instance the dialog could be already created and could be cached,
    // in which case you want to display the cached dialog
    if ( PreferencesDialog::showDialog( "preferences" ) )
        return;

    // we didn't find an instance of this dialog, so lets create it
    PreferencesDialog * dialog = new PreferencesDialog( m_pageView, Settings::self() );
    // keep us informed when the user changes settings
    connect( dialog, SIGNAL( settingsChanged() ), this, SLOT( slotNewConfig() ) );

    dialog->show();
}

void Part::slotNewConfig()
{
    // Apply settings here. A good policy is to check wether the setting has
    // changed before applying changes.

    // Watch File
    bool watchFile = Settings::watchFile();  
    if ( watchFile && m_watcher->isStopped() )
        m_watcher->startScan();
    if ( !watchFile && !m_watcher->isStopped() )
    {
        m_dirtyHandler->stop();
        m_watcher->stopScan();
    }

    bool showSearch = Settings::showSearchBar();
    if ( m_searchWidget->isShown() != showSearch )
        m_searchWidget->setShown( showSearch );

    // Main View (pageView)
    QScrollView::ScrollBarMode scrollBarMode = Settings::showScrollBars() ?
        QScrollView::AlwaysOn : QScrollView::AlwaysOff;
    if ( m_pageView->hScrollBarMode() != scrollBarMode )
    {
        m_pageView->setHScrollBarMode( scrollBarMode );
        m_pageView->setVScrollBarMode( scrollBarMode );
    }

    // update document settings
    m_document->reparseConfig();

    // update Main View and ThumbnailList contents
    // TODO do this only when changing Settings::renderMode()
    m_pageView->updateContents();
    if ( Settings::showLeftPanel() && m_thumbnailList->isShown() )
        m_thumbnailList->updateWidgets();
}

void Part::slotPrintPreview()
{
    if (m_document->pages() == 0) return;

    double width, height;
    int landscape, portrait;
    KPrinter printer;
    const KPDFPage *page;

    printer.setMinMax(1, m_document->pages());
    printer.setPreviewOnly( true );
    printer.setMargins(0, 0, 0, 0);

    // if some pages are landscape and others are not the most common win as kprinter does
    // not accept a per page setting
    landscape = 0;
    portrait = 0;
    for (uint i = 0; i < m_document->pages(); i++)
    {
        page = m_document->page(i);
        width = page->width();
        height = page->height();
        if (page->rotation() == 90 || page->rotation() == 270) qSwap(width, height);
        if (width > height) landscape++;
        else portrait++;
    }
    if (landscape > portrait) printer.setOption("orientation-requested", "4");

    doPrint(printer);
}

void Part::slotShowMenu(const KPDFPage *page, const QPoint &point)
{
	bool reallyShow = false;
	if (!m_actionsSearched)
	{
		// the quest for options_show_menubar
		KXMLGUIClient *client;
		KActionCollection *ac;
		KActionPtrList::const_iterator it, end, begin;
		KActionPtrList actions;
		
		if (factory())
		{
			QPtrList<KXMLGUIClient> clients(factory()->clients());
			QPtrListIterator<KXMLGUIClient> clientsIt( clients );
			for( ; (!m_showMenuBarAction || !m_showFullScreenAction) && clientsIt.current(); ++clientsIt)
			{
				client = clientsIt.current();
				ac = client->actionCollection();
				actions = ac->actions();
				end = actions.end();
				begin = actions.begin();
				for ( it = begin; it != end; ++it )
				{
					if (QString((*it)->name()) == "options_show_menubar") m_showMenuBarAction = (KToggleAction*)(*it);
					if (QString((*it)->name()) == "fullscreen") m_showFullScreenAction = (KToggleAction*)(*it);
				}
			}
		}
		m_actionsSearched = true;
	}
	
	
	KPopupMenu *popup = new KPopupMenu( widget(), "rmb popup" );
	if (page)
	{
		popup->insertTitle( i18n( "Page %1" ).arg( page->number() + 1 ) );
        if ( page->hasBookmark() )
			popup->insertItem( SmallIcon("bookmark"), i18n("Remove Bookmark"), 1 );
		else
			popup->insertItem( SmallIcon("bookmark_add"), i18n("Add Bookmark"), 1 );
		if ( m_pageView->canFitPageWidth() )
			popup->insertItem( SmallIcon("viewmagfit"), i18n("Fit Width"), 2 );
		//popup->insertItem( SmallIcon("pencil"), i18n("Edit"), 3 );
		//popup->setItemEnabled( 3, false );
		reallyShow = true;
	}
/*
	//Albert says: I have not ported this as i don't see it does anything
    if ( d->mouseOnRect ) // and rect->objectType() == ObjectRect::Image ...
	{
		m_popup->insertItem( SmallIcon("filesave"), i18n("Save Image..."), 4 );
		m_popup->setItemEnabled( 4, false );
}*/
	
	if ((m_showMenuBarAction && !m_showMenuBarAction->isChecked()) || (m_showFullScreenAction && m_showFullScreenAction->isChecked()))
	{
		popup->insertTitle( i18n( "Tools" ) );
		if (m_showMenuBarAction && !m_showMenuBarAction->isChecked()) m_showMenuBarAction->plug(popup);
		if (m_showFullScreenAction && m_showFullScreenAction->isChecked()) m_showFullScreenAction->plug(popup);
		reallyShow = true;
		
	}
	
	if (reallyShow)
	{
		switch ( popup->exec(point) )
		{
			case 1:
				m_document->toggleBookmark( page->number() );
				break;
			case 2:
				m_pageView->fitPageWidth( page->number() );
				break;
	//		case 3: // switch to edit mode
	//			break;
		}
	}
	delete popup;
}

void Part::slotShowProperties()
{
	PropertiesDialog *d = new PropertiesDialog(widget(), m_document);
	d->exec();
	delete d;
}

void Part::slotShowPresentation()
{
    if ( !m_presentationWidget )
      m_presentationWidget = new PresentationWidget( widget(), m_document );
}

void Part::slotPrint()
{
    if (m_document->pages() == 0) return;

    double width, height;
    int landscape, portrait;
    KPrinter printer;
    const KPDFPage *page;

    printer.setPageSelection(KPrinter::ApplicationSide);
    printer.setMinMax(1, m_document->pages());
    printer.setCurrentPage(m_document->currentPage()+1);
    printer.setMargins(0, 0, 0, 0);

    // if some pages are landscape and others are not the most common win as kprinter does
    // not accept a per page setting
    landscape = 0;
    portrait = 0;
    for (uint i = 0; i < m_document->pages(); i++)
    {
        page = m_document->page(i);
        width = page->width();
        height = page->height();
        if (page->rotation() == 90 || page->rotation() == 270) qSwap(width, height);
        if (width > height) landscape++;
        else portrait++;
    }
    if (landscape > portrait) printer.setOrientation(KPrinter::Landscape);

    if (printer.setup(widget())) doPrint( printer );
}

void Part::doPrint(KPrinter &printer)
{
    if (!m_document->isAllowed(KPDFDocument::AllowPrint))
    {
        KMessageBox::error(widget(), i18n("Printing this document is not allowed."));
        return;
    }

    if (!m_document->print(printer))
    {
        KMessageBox::error(widget(), i18n("Could not print the document. Please report to bugs.kde.org"));	
    }
}

void Part::restoreDocument(const KURL &url, int page)
{
  if (openURL(url)) goToPage(page);
}

void Part::saveDocumentRestoreInfo(KConfig* config)
{
  config->writePathEntry( "URL", url().url() );
  if (m_document->pages() > 0) config->writeEntry( "Page", m_document->currentPage() + 1 );
}

/*
* BrowserExtension class
*/
BrowserExtension::BrowserExtension(Part* parent)
  : KParts::BrowserExtension( parent, "KPDF::BrowserExtension" )
{
	emit enableAction("print", true);
	setURLDropHandlingEnabled(true);
}

void BrowserExtension::print()
{
	static_cast<Part*>(parent())->slotPrint();
}

#include "part.moc"
