/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|							 (C) SuSE GmbH |
\----------------------------------------------------------------------/

  File:	      YQApplication.cc

  Author:     Stefan Hundhammer <sh@suse.de>

/-*/


#include <qapp.h>
#include <qlocale.h>
#include <qregexp.h>

#define y2log_component "qt-ui"
#include <ycp/y2log.h>

#include "YQApplication.h"


YQApplication::YQApplication()
    : YApplication()
    , _currentFont( 0 )
    , _headingFont( 0 )
    , _boldFont( 0 )
    , _fontFamily( "Sans Serif" )
    , _langFonts( 0 )
    , _qtTranslations( 0 )
    , _autoFonts( false )
    , _autoNormalFontSize( -1 )
    , _autoHeadingFontSize( -1 )
{
    y2debug( "YQApplication constructor start" );
    
    setIconBasePath( ICONDIR "/icons/22x22/apps/" );
    loadPredefinedQtTranslations();
    
    y2debug( "YQApplication constructor end" );
}


YQApplication::~YQApplication()
{
    if ( _langFonts )
	delete _langFonts;

    if ( _qtTranslations )
	delete _qtTranslations;

    deleteFonts();
}


void
YQApplication::setLanguage( const string & language,
			    const string & encoding )
{
    YApplication::setLanguage( language, encoding );
    loadPredefinedQtTranslations();
    setLangFonts( language, encoding );
}



void
YQApplication::loadPredefinedQtTranslations()
{
    QString path = QT_LOCALEDIR;
    QString language = QLocale::system().name();

    QString transFile = QString( "qt_%1.qm")
              .arg( language.lower().replace('_','-') );

    if ( path.isEmpty() )
    {
	y2warning( "Qt locale directory not set - "
		   "no translations for predefined Qt dialogs" );
	return;
    }

    if ( ! _qtTranslations )
	_qtTranslations = new QTranslator();
    
    _qtTranslations->load( transFile, path );

    if ( _qtTranslations->isEmpty() )
    {
	// try fallback
	transFile = QString( "qt_%1.qm").arg( language.lower().left(2) );
	_qtTranslations->load( transFile, path );
    }

    if ( _qtTranslations->isEmpty() )
    {
	y2warning( "Can't load translations for predefined Qt dialogs from %s/%s",
		   (const char *) path, (const char *) transFile );
    }
    else
    {
	y2milestone( "Loaded translations for predefined Qt dialogs from %s/%s",
		     (const char *) path, (const char *) transFile );

	qApp->installTranslator( _qtTranslations );
    }


    // Force reverse layout for Arabic and Hebrew

    if ( ( language.startsWith( "ar" ) ||	// Arabic
	   language.startsWith( "he" ) )	// Hebrew
	 && ! qApp->reverseLayout() )
    {
	y2warning( "Using fallback rule for reverse layout for language '%s'",
		   (const char *) language );

	qApp->setReverseLayout( true );
    }
}


void
YQApplication::setLangFonts( const string & language, const string & encoding )
{
    QString oldFontFamily = _fontFamily;

    if ( ! _langFonts )
    {
	_langFonts = new QY2Settings( LANG_FONTS_FILE );
	CHECK_PTR( _langFonts );

	if ( _langFonts->readError() )
	    y2error( "Error reading %s", (const char *) _langFonts->fileName() );
	else
	    y2milestone( "%s read OK", (const char *) _langFonts->fileName() );
    }

    QString lang = language.c_str();
    
    if ( ! encoding.empty() )
	lang += QString( "." ) + encoding.c_str();
    
    QString key;

    if ( ! _langFonts->hasKey( fontKey( lang ) ) )	// Try with encoding ("zh_CN.UTF8" etc.)
    {
	lang = language.c_str();			// Try without encoding ("zh_CN")

	if ( ! _langFonts->hasKey( fontKey( lang ) ) )	
	    lang.replace( QRegExp( "_.*$" ), "" );	// Cut off trailing country ("_CN")
    }

    if ( _langFonts->hasKey( fontKey( lang ) ) )
    {
	_fontFamily = _langFonts->get( fontKey( lang ), "Sans Serif" );
	y2milestone( "%s = \"%s\"", (const char *) fontKey( lang ), (const char *) _fontFamily );
    }
    else
    {
	_fontFamily = _langFonts->get( fontKey( "" ), "Sans Serif" );
	y2milestone( "Using fallback for %s: font = \"%s\"",
		     (const char *) lang, (const char *) _fontFamily );
    }

    if ( _fontFamily != oldFontFamily && ! _fontFamily.isEmpty() )
    {
	y2milestone( "New font family: %s", _fontFamily.latin1() );
	deleteFonts();
	int size = qApp->font().pointSize();
	QFont font( _fontFamily );
	font.setPointSize( size );
	qApp->setFont( font, true );	// font, informWidgets
	y2milestone( "Reloading fonts - now using \"%s\"",
		     (const char *) font.toString() );
    }
    else
    {
	y2debug( "No font change" );
    }
}


QString
YQApplication::fontKey( const QString & lang )
{
    if ( lang.isEmpty() )
	return "font";
    else
	return QString( "font[%1]").arg( lang );
}


const QFont &
YQApplication::currentFont()
{
    /**
     * Brute force approach to make sure we'll really get a complete Unicode font:
     * Explicitly load the one font that we made sure to contain all required
     * characters, including Latin1, Latin2, Japanese, Korean, and the
     * characters used for glyphs.
     *
     * There are many fonts that claim to be Unicode, but most of them contain
     * just a sorry excuse for a complete Unicode character set. Qt can't know
     * how complete a font is, so it chooses one that might be better in otherf
     * aspects, but lacks necessary characters.
     **/

    if ( ! _currentFont )
    {
	if ( autoFonts() )
	{
	    pickAutoFonts();

	    _currentFont = new QFont( _fontFamily );
	    _currentFont->setPixelSize( _autoNormalFontSize );
	    _currentFont->setWeight( QFont::Normal );

	    y2milestone( "Loaded %d pixel font: %s", _autoNormalFontSize,
			 (const char *) _currentFont->toString() );

	    qApp->setFont( * _currentFont, true );	// font, informWidgets
	}
	else
	{
	    // y2debug( "Copying QApplication::font()" );
	    _currentFont = new QFont( qApp->font() );
	}
    }

    return * _currentFont;
}


const QFont &
YQApplication::boldFont()
{
    if ( ! _boldFont )
    {
	_boldFont = new QFont( currentFont() );
	_boldFont->setBold( true );
    }

    return * _boldFont;
}


const QFont &
YQApplication::headingFont()
{
    /**
     * Brute force load the heading font - see currentFont() above for more.
     **/

    if ( ! _headingFont )
    {
	if ( autoFonts() )
	{
	    pickAutoFonts();

	    _headingFont = new QFont( _fontFamily );
	    _headingFont->setPixelSize( _autoHeadingFontSize );
	    _headingFont->setWeight( QFont::Bold );

	    y2milestone( "Loaded %d pixel bold font: %s", _autoHeadingFontSize,
			 (const char *) _headingFont->toString() );
	}
	else
	{
	    _headingFont = new QFont( _fontFamily, 14, QFont::Bold );
	}
    }

    return * _headingFont;
}


void
YQApplication::deleteFonts()
{
    if ( _currentFont )
	delete _currentFont;

    if ( _headingFont )
	delete _headingFont;

    if ( _boldFont )
	delete _boldFont;

    _currentFont = 0;
    _headingFont = 0;
    _boldFont    = 0;
}


void
YQApplication::setAutoFonts( bool useAutoFonts )
{
    _autoFonts = useAutoFonts;
}


void
YQApplication::pickAutoFonts()
{
    if ( _autoNormalFontSize >= 0 )	// Use cached values
	return;

#warning FIXME: defaultSize
#if 0
    int x = _default_size.width();
    int y = _default_size.height();
#endif
    int x = 800;
    int y = 600;
	

    int normal  = 10;
    int heading	= 12;

    if ( x >= 800 && y >= 600 )
    {
	normal	= 10;
	heading	= 12;
    }

    if ( x >= 1024 && y >= 768 )
    {
	normal	= 12;
	heading	= 14;
    }

    if ( x >= 1280 && y >= 1024 )
    {
	normal	= 14;
	heading	= 18;
    }

    if ( x >= 1400 )
    {
	normal	= 16;
	heading	= 20;
    }

    if ( x >= 1600 )
    {
	normal	= 18;
	heading	= 24;
    }

    if ( x >= 2048 )	// Sounds futuristic? Just wait one or two years...
    {
	normal	= 20;
	heading	= 28;
    }

    _autoNormalFontSize  = normal;
    _autoHeadingFontSize = heading;

    y2milestone( "Selecting auto fonts - normal: %d, heading: %d (bold)",
		 _autoNormalFontSize, _autoHeadingFontSize );
}

    


#include "YQApplication.moc"
