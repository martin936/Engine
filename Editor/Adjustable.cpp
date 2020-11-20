#include "Game.h"
#include <time.h>
#include <cuda_runtime.h>
#include <nanogui/nanogui.h>
#include <iostream>
#include <stdio.h>
#include "Editor.h"



Adjustable::Adjustable( nanogui::Widget* parent, std::string name, float* ptr, float min, float max )
{
	m_name 		= name;
	m_dataptr 	= ptr;
	m_idataptr  = NULL;
	m_minvalue 	= min;
	m_maxvalue 	= max;
	m_value 	= *ptr;
	char str[256] = "";

	m_pwidget_parent = parent;

	m_pwidget_layout = new nanogui::Widget(parent);
	m_pwidget_layout->setLayout( new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 0, 20) );

	m_pwidget_label = new nanogui::Label(m_pwidget_layout, name.c_str(), "sans-bold");
	m_pwidget_label->setFixedWidth(120);

	m_pwidget_slider = new nanogui::Slider(m_pwidget_layout);
	m_pwidget_slider->setValue((m_value - m_minvalue)/(m_maxvalue-m_minvalue));
	m_pwidget_slider->setFixedWidth(120);

	m_pwidget_textbox = new nanogui::TextBox( m_pwidget_layout );
	sprintf( str, "%0.2f", *ptr );
	m_pwidget_textbox->setValue(str);

	m_pwidget_slider->setCallback([this](float value) {
		char str[256] = "";
		m_value = value*(m_maxvalue-m_minvalue) + m_minvalue;
		sprintf( str, "%0.2f", m_value );
		m_pwidget_textbox->setValue(str);
	});

}


Adjustable::Adjustable( nanogui::Widget* parent, std::string name, int* ptr, int min, int max )
{
	m_name 		= name;
	m_idataptr 	= ptr;
	m_dataptr   = NULL;
	m_minvalue 	= (float)min;
	m_maxvalue 	= (float)max;
	m_value 	= (float)*ptr;
	char str[256] = "";

	m_pwidget_parent = parent;

	m_pwidget_layout = new nanogui::Widget(parent);
	m_pwidget_layout->setLayout( new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 0, 20) );

	m_pwidget_label = new nanogui::Label(m_pwidget_layout, name.c_str(), "sans-bold");
	m_pwidget_label->setFixedWidth(120);

	m_pwidget_slider = new nanogui::Slider(m_pwidget_layout);
	m_pwidget_slider->setValue((m_value - m_minvalue)/(m_maxvalue-m_minvalue));
	m_pwidget_slider->setFixedWidth(120);

	m_pwidget_textbox = new nanogui::TextBox( m_pwidget_layout );
	sprintf( str, "%d", *ptr );
	m_pwidget_textbox->setValue(str);

	m_pwidget_slider->setCallback([this](float value) {
		char str[256] = "";
		m_value = value*(m_maxvalue-m_minvalue) + m_minvalue;
		sprintf( str, "%d", (int)m_value );
		m_pwidget_textbox->setValue(str);
	});

}


Adjustable::~Adjustable( void )
{
}

void Adjustable::reset( void )
{
	char str[256] = "";

	if( m_dataptr != NULL )
	{
		m_value = *m_dataptr;
		sprintf( str, "%0.2f", m_value );
	}
	else if( m_idataptr != NULL )
	{
		m_value = (float)*m_idataptr;
		sprintf( str, "%d", (int)m_value );
	}

	m_pwidget_slider->setValue((m_value - m_minvalue)/(m_maxvalue-m_minvalue));
	m_pwidget_textbox->setValue(str);
}