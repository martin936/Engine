#include "Game.h"
#include <time.h>
#include <nanogui/nanogui.h>
#include <iostream>
#include "Editor.h"


Category::Category( nanogui::Widget* parent, std::string name )
{
	m_pwidget_parent = parent;

	m_pwidget_popup_button = new nanogui::PopupButton( parent, name.c_str(), ENTYPO_ICON_PENCIL );
	nanogui::Popup* popup = m_pwidget_popup_button->popup();

	popup->setLayout( new nanogui::GroupLayout() );

	m_pwidget_label = new nanogui::Label( popup, name, "sans-bold" );

}


void Category::addAdjustable( std::string name, float* ptr, float min, float max )
{
	m_adjustables.push_back( new Adjustable(m_pwidget_popup_button->popup(), name, ptr, min, max) );
}

void Category::addAdjustable( std::string name, int* ptr, int min, int max )
{
	m_adjustables.push_back( new Adjustable(m_pwidget_popup_button->popup(), name, ptr, min, max) );
}


Category::~Category( void )
{
}

void Category::reset( void )
{
	std::vector<Adjustable*>::iterator it;
	for( it = m_adjustables.begin() ; it < m_adjustables.end() ; it++ )
		(*it)->reset();
}


void Category::addApplyButton( void )
{
	nanogui::Button* button = new nanogui::Button( m_pwidget_popup_button->popup(), "Apply" );
	button->setCallback( [this] { 
		std::vector<Adjustable*>::iterator it;
		for( it = m_adjustables.begin() ; it < m_adjustables.end() ; it++ )
			(*it)->flush(); 
	} );
}


void Category::addApplyInfoButton( void )
{
	nanogui::Button* button = new nanogui::Button( m_pwidget_popup_button->popup(), "Apply" );
	button->setCallback( [this] { 
		std::vector<Adjustable*>::iterator it;
		for( it = m_adjustables.begin() ; it < m_adjustables.end() ; it++ )
			(*it)->flush(); 

		auto dlg = new nanogui::MessageDialog( _screen_, nanogui::MessageDialog::Type::Warning, "Apply Changes", "In order for your changes to take effect, you need to refresh the player by pressing Select or R.");
		dlg->center();
	} );
}