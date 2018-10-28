#include "setting.h"
#include "ui_setting.h"

Setting::Setting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Setting)
{
    ui->setupUi(this);

    connect(ui->CHNSlider,SIGNAL(valueChanged(int)),ui->CHNlcd,SLOT(display(int)));
}

Setting::~Setting()
{
    delete ui;
}
