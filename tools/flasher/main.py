import sys
import logging
from PyQt5.QtWidgets import (QApplication, QMainWindow, QTabWidget)
from security_tab import SecurityTab
from main_tab import MainTab


logging.basicConfig(level=logging.INFO, filename='flasher.log', filemode='a',
                    format='%(asctime)s - %(levelname)s - %(message)s')

class BeeComFlasher(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Flasher')
        self.setGeometry(100, 100, 450, 600)
        self.setupTabs()
        self.show()

    def setupTabs(self):
        tab_widget = QTabWidget(self)
        self.setCentralWidget(tab_widget)
        tab_widget.addTab(MainTab(), "Software Download")
        tab_widget.addTab(SecurityTab(), "Security")


if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = BeeComFlasher()
    sys.exit(app.exec_())
