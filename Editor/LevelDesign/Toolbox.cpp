#include "Engine/Editor/Editor.h"
#include "Engine/Editor/Externalized/Externalized.h"

EXPORT_EXTERN(int, gs_nWindowWidth)

CToolBox::CToolBox()
{
	/*m_pScreen = pScreen;

	nanogui::Window *window = new nanogui::Window(m_pScreen, "Tool Box");
	window->setLayout(new nanogui::GroupLayout());*/

	m_pCollisionBoxManager = new CCollisionBoxManager;

	/*nanogui::PopupButton* popup_button = new nanogui::PopupButton(window, "Collision Boxes Editor");
	nanogui::Popup* popup = popup_button->popup();

	popup->setLayout(new nanogui::GroupLayout());
	nanogui::Label* label = new nanogui::Label(popup, "Collision Boxes Editor");

	nanogui::Button* AddButton = new nanogui::Button(popup, "New Box");
	AddButton->setCallback([this] {
		GetCollisionBoxManager()->AddBox();
	});

	nanogui::Button* RemoveButton = new nanogui::Button(popup, "Remove Box");
	RemoveButton->setCallback([this] {
		GetCollisionBoxManager()->RemoveSelectedBox();
	});

	nanogui::Button* LoadButton = new nanogui::Button(popup, "Load");
	LoadButton->setCallback([this] {
		TCHAR pwd[256];
		GetCurrentDirectory(256, pwd);

		std::string file = nanogui::file_dialog({ { "cbf", "Collision Box File" } }, false);

		SetCurrentDirectory(pwd);

		GetCollisionBoxManager()->Load(file.c_str());


	});

	nanogui::Button* SaveButton = new nanogui::Button(popup, "Save");
	SaveButton->setCallback([this] {
		TCHAR pwd[256];
		GetCurrentDirectory(256, pwd);

		std::string file = nanogui::file_dialog({ { "cbf", "Collision Box File" } }, true);

		SetCurrentDirectory(pwd);

		GetCollisionBoxManager()->Save(file.c_str());
	});

	window->setFixedWidth(300);
	window->setPosition(Eigen::Vector2i(gs_nWindowWidth / 2 - 150, 0));*/
}


void CToolBox::Update()
{
	/*if (CEditor::ShouldUpdate())
		m_pCollisionBoxManager->Update();

	m_pCollisionBoxManager->Draw();*/
}

CToolBox::~CToolBox()
{
	delete m_pCollisionBoxManager;
}