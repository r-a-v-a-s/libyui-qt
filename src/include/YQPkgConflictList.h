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

  File:	      YQPkgConflictList.h

  Author:     Stefan Hundhammer <sh@suse.de>

/-*/

// -*- c++ -*-


#ifndef YQPkgConflictList_h
#define YQPkgConflictList_h

#include <y2pm/PkgDep.h>
#include "QY2ListView.h"


class YQPkgConflictAlternative;
class YQPkgConflictResolution;


/**
 * @short Display package dependency conflicts in a tree list and let the user
 * choose how to resolve each conflict.
 **/
class YQPkgConflictList : public QY2ListView
{
    Q_OBJECT

public:
    /**
     * Constructor.
     **/
    YQPkgConflictList( QWidget * parent );

    /**
     * Destructor.
     **/
    virtual ~YQPkgConflictList();

    /**
     * Fill the list with the specified bad list.
     **/
    void fill( PkgDep::ErrorResultList & badList );

    /**
     * Check if the user choices are complete.
     **/
    bool choicesComplete();

public slots:

    /**
     * Activate the choices the user made.
     **/
    void activateUserChoices();


};



/**
 * @short Root item for each individual conflict
 **/
class YQPkgConflict: public QY2ListViewItem
{
public:

    /**
     * Constructor.
     **/
    YQPkgConflict( YQPkgConflictList *		parentList,
		   const PkgDep::ErrorResult &	errorResult );

    /**
     * Destructor.
     **/
    virtual ~YQPkgConflict() {}

    /**
     * Check if this conflict is resolved, i.e. the user made a choice how to
     * deal with this conflict.
     **/
    bool isResolved();

    /**
     * Access the internal ErrorResult.
     **/
    PkgDep::ErrorResult & errorResult() { return _conflict; }

protected:

    /**
     * Format the text line for this item.
     **/
    void formatLine();
    

    PMObjectPtr			_pmObj;
    QString			_shortName;	// Only pkg name (no version)
    QString			_fullName;	// Name + edition
    PMSelectable::UI_Status	_status;

    bool			_collision;
    bool			_needAlternative;

    YQPkgConflictAlternative *	_firstAlternative;
    YQPkgConflictResolution *	_firstResolution;
    
    PkgDep::ErrorResult		_conflict;
    YQPkgConflictList *		_parentList;
};



/**
 * @short Generic conflict list item with a simple text
 **/
class YQPkgConflictTextItem: public QY2ListViewItem
{
public:

    /**
     * Constructor.
     **/
    
    YQPkgConflictTextItem( QListViewItem * 	parent,
			   const QString & 	text,
			   bool			sortByInsertionOrder = true )
	: QY2ListViewItem( parent, sortByInsertionOrder )
    {
	setText( 0, text );    
    }


    /**
     * Destructor.
     **/
    virtual ~YQPkgConflictTextItem() {}
};



class YQPkgConflictAlternative: public QY2CheckListItem
{
    YQPkgConflictAlternative( QListViewItem * parent, const QString & text )
	: QY2CheckListItem( parent, text, QCheckListItem::RadioButton )
	{}
};


class YQPkgConflictResolution: public QY2CheckListItem
{
    YQPkgConflictResolution( QListViewItem * parent, const QString & text )
	: QY2CheckListItem( parent, text, QCheckListItem::RadioButton )
	{}
};


#endif // ifndef YQPkgConflictList_h
