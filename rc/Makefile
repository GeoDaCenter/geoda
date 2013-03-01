all: MyAppResources.cpp MyDataViewerAppRes.cpp

MyAppResources.cpp: dialogs.xrc data_viewer_dialogs.xrc menus.xrc toolbar.xrc panel.xrc
	wxrc dialogs.xrc data_viewer_dialogs.xrc menus.xrc toolbar.xrc panel.xrc --cpp-code --output=MyAppResources.cpp --function=MyInitXmlResource
MyDataViewerAppRes.cpp: data_viewer_dialogs.xrc
	wxrc data_viewer_dialogs.xrc --cpp-code --output=MyDataViewerAppRes.cpp --function=DataViewerInitXmlResource