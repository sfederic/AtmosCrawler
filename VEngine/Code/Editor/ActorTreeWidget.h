#pragma once
#include <qtreewidget.h>

class QDropEvent;
class QDragMoveEvent;
class QDragEnterEvent;
class Actor;

struct ActorTreeWidget : public QTreeWidget
{
private:
	Actor* dragChildActor = nullptr;

public:
	ActorTreeWidget(QWidget* parent = nullptr);
	
	//Disables key press searching in treewidgets by redefining as empty.
	virtual void keyboardSearch(const QString& search) {};

protected:
	//Events called when actors in world list are dragged and dropped on each other in list.
	virtual void dropEvent(QDropEvent* event) override;
	virtual void dragEnterEvent(QDragEnterEvent* event) override;
};
