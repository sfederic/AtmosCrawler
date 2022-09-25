#include "vpch.h"
#include "IntWidget.h"

IntWidget::IntWidget(Property& prop_, bool readOnly, QWidget* parent) : IntSpinBox(parent)
{
	prop = prop_;
	_value = prop.GetData<int>();

	if (readOnly)
	{
		setDisabled(true);
	}

	connect(this, &QDoubleSpinBox::editingFinished, this, &IntWidget::SetValue);
	setValue(*_value);
}

void IntWidget::SetValue()
{
	*_value = value();
}

void IntWidget::ResetValue()
{
	setValue(*_value);
}
