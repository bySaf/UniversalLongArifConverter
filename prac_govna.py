import sys
import socket
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QLabel, QLineEdit, QTextEdit, QPushButton,
    QVBoxLayout, QHBoxLayout, QFileDialog, QMessageBox, QProgressBar, QFrame, QTabWidget
)
from PyQt5.QtCore import Qt, pyqtSignal, QThread
from PyQt5.QtGui import QFont, QIcon

class NetworkThread(QThread):
    result_ready = pyqtSignal(str)
    error_occurred = pyqtSignal(str)
    connection_status = pyqtSignal(str, str)

    def __init__(self, typy, float_value, int_value1, int_value2, operation = None):
        super().__init__()
        if typy == "convert":
            self.type = "convert"
            self.float_value = float_value
            self.int_value1 = int_value1
            self.int_value2 = int_value2
        else:
            self.type = "arif"
            self.float_value1 = float_value
            self.float_value2 = int_value1
            self.base = int_value2
            self.operation = operation


    def run(self):
        try:
            self.connection_status.emit("Подключение...", "orange")

            if (self.type == "convert"):
                data_to_send = f"{self.type},{self.float_value},{self.int_value1},{self.int_value2}"
            elif (self.type == "arif"):
                data_to_send = f"{self.type},{self.float_value1},{self.float_value2},{self.base},{self.operation}"



            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(30)
                s.connect(('localhost', 65432))
                s.sendall(data_to_send.encode('utf-8'))
                response = s.recv(1000000).decode('utf-8')

            self.result_ready.emit(response)
            self.connection_status.emit("Соединение установлено", "green")
        except socket.timeout:
            self.error_occurred.emit("Не удалось подключиться к серверу. Таймаут.")
            self.connection_status.emit("Соединение не установлено", "red")
        except ConnectionRefusedError:
            self.error_occurred.emit("Сервер недоступен.")
            self.connection_status.emit("Соединение не установлено", "red")
        except Exception as e:
            self.error_occurred.emit(f"Произошла ошибка: {e}")
            self.connection_status.emit("Соединение не установлено", "red")

class ConverterWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Конвертер Чисел")
        self.setFixedSize(700, 700)
        self.setWindowIcon(QIcon("icon.png"))
        self.init_ui()

    def init_ui(self):
        # Устанавливаем темную тему
        self.setStyleSheet("""
            QMainWindow {
                background-color: #2b2b2b;
            }
            QLabel {
                color: #ffffff;
            }
            QLineEdit, QTextEdit {
                background-color: #3c3f41;
                color: #ffffff;
                border: 1px solid #555555;
                border-radius: 4px;
                padding: 6px;
            }
            QPushButton {
                background-color: #3c3f41;
                color: #ffffff;
                border: 1px solid #555555;
                border-radius: 4px;
                padding: 8px;
                min-width: 80px;
            }
            QPushButton:hover {
                background-color: #505357;
            }
            QProgressBar {
                background-color: #3c3f41;
                color: #ffffff;
                border: 1px solid #555555;
                border-radius: 4px;
                text-align: center;
            }
            QTextEdit[readOnly="true"] {
                background-color: #3c3f41;
                color: #ffffff;
            }
        """)

        # Основной виджет и макет
        main_widget = QWidget()
        main_layout = QVBoxLayout()
        main_widget.setLayout(main_layout)
        self.setCentralWidget(main_widget)

        # Создаем вкладки
        self.tabs = QTabWidget()
        main_layout.addWidget(self.tabs)

        # Первая вкладка - Конвертация чисел
        self.init_conversion_tab()
        self.tabs.addTab(self.conversion_tab, "Конвертация")

        # Вторая вкладка - Арифметические операции
        self.init_arithmetic_tab()
        self.tabs.addTab(self.arithmetic_tab, "Арифметика")

        # Заполняем пространство
        main_layout.addStretch()

    def init_conversion_tab(self):
        self.conversion_tab = QWidget()
        layout = QVBoxLayout()
        self.conversion_tab.setLayout(layout)

        # Заголовок
        title_label = QLabel("Конвертер Чисел")
        title_font = QFont("Helvetica", 32, QFont.Bold)
        title_label.setFont(title_font)
        title_label.setAlignment(Qt.AlignCenter)
        layout.addWidget(title_label)

        # Статус подключения
        self.connection_label = QLabel("Соединение не установлено")
        connection_font = QFont("Helvetica", 14)
        self.connection_label.setFont(connection_font)
        self.connection_label.setAlignment(Qt.AlignCenter)
        self.connection_label.setStyleSheet("color: red;")
        layout.addWidget(self.connection_label)

        # Разделитель
        separator = QFrame()
        separator.setFrameShape(QFrame.HLine)
        separator.setFrameShadow(QFrame.Sunken)
        layout.addWidget(separator)

        # Поля ввода
        self.entry_float = self.create_input_field("Число для перевода:", multiline=True)
        self.entry_int1 = self.create_input_field("Исходная система:")
        self.entry_int2 = self.create_input_field("Целевая система:")

        layout.addWidget(self.entry_float)
        layout.addWidget(self.entry_int1)
        layout.addWidget(self.entry_int2)

        # Область вывода результата
        response_label = QLabel("Результат:")
        response_font = QFont("Helvetica", 18)
        response_label.setFont(response_font)
        layout.addWidget(response_label)

        self.text_response = QTextEdit()
        self.text_response.setReadOnly(True)
        self.text_response.setFixedHeight(150)
        layout.addWidget(self.text_response)

        # Индикатор прогресса
        self.progress_bar = QProgressBar()
        self.progress_bar.setTextVisible(False)
        self.progress_bar.setVisible(False)
        layout.addWidget(self.progress_bar)

        # Кнопки
        button_layout = QHBoxLayout()
        self.load_button = QPushButton("Загрузить")
        self.load_button.clicked.connect(self.load_from_file)
        button_layout.addWidget(self.load_button)

        self.send_button = QPushButton("Конвертировать")
        self.send_button.clicked.connect(self.send_data)
        button_layout.addWidget(self.send_button)

        self.save_button = QPushButton("Сохранить")
        self.save_button.clicked.connect(self.save_response)
        button_layout.addWidget(self.save_button)

        layout.addLayout(button_layout)

    def init_arithmetic_tab(self):
        self.arithmetic_tab = QWidget()
        layout = QVBoxLayout()
        self.arithmetic_tab.setLayout(layout)

        # Заголовок
        title_label = QLabel("Арифметические операции")
        title_font = QFont("Helvetica", 32, QFont.Bold)
        title_label.setFont(title_font)
        title_label.setAlignment(Qt.AlignCenter)
        layout.addWidget(title_label)

        # Поля ввода
        self.entry_num1 = self.create_input_field("Первое число:")
        self.entry_num2 = self.create_input_field("Второе число:")
        self.entry_operation = self.create_input_field("Операция (+, -, *, /):")
        self.base = self.create_input_field("Cистема счисления")

        layout.addWidget(self.entry_num1)
        layout.addWidget(self.entry_num2)
        layout.addWidget(self.entry_operation)
        layout.addWidget(self.base)

        # Область вывода результата
        response_label = QLabel("Результат:")
        response_font = QFont("Helvetica", 18)
        response_label.setFont(response_font)
        layout.addWidget(response_label)

        self.text_response_arithmetic = QTextEdit()
        self.text_response_arithmetic.setReadOnly(True)
        self.text_response_arithmetic.setFixedHeight(150)
        layout.addWidget(self.text_response_arithmetic)

        # Индикатор прогресса
        self.progress_bar_arithmetic = QProgressBar()
        self.progress_bar_arithmetic.setTextVisible(False)
        self.progress_bar_arithmetic.setVisible(False)
        layout.addWidget(self.progress_bar_arithmetic)

        # Кнопка для выполнения операции
        self.calculate_button = QPushButton("Выполнить")
        self.calculate_button.clicked.connect(self.perform_arithmetic_operation)
        layout.addWidget(self.calculate_button)

    def create_input_field(self, label_text, multiline=False):
        container = QWidget()
        layout = QVBoxLayout()
        container.setLayout(layout)

        label = QLabel(label_text)
        label_font = QFont("Helvetica", 16)
        label.setFont(label_font)
        layout.addWidget(label)

        if multiline:
            entry = QTextEdit()
            entry.setFixedHeight(70)
        else:
            entry = QLineEdit()
            entry.setPlaceholderText("Введите значение")
        layout.addWidget(entry)

        return container

    def get_entry_value(self, container):
        entry = container.layout().itemAt(1).widget()
        if isinstance(entry, QTextEdit):
            return entry.toPlainText().strip()
        elif isinstance(entry, QLineEdit):
            return entry.text().strip()
        return ""

    def set_entry_value(self, container, value):
        entry = container.layout().itemAt(1).widget()
        if isinstance(entry, QTextEdit):
            entry.setPlainText(value)
        elif isinstance(entry, QLineEdit):
            entry.setText(value)

    def send_data(self):
        float_value = self.get_entry_value(self.entry_float)
        int_value1 = self.get_entry_value(self.entry_int1)
        int_value2 = self.get_entry_value(self.entry_int2)

        if not float_value or not int_value1 or not int_value2:
            QMessageBox.warning(self, "Внимание", "Пожалуйста, заполните все поля.")
            return

        self.start_loading()

        self.thread = NetworkThread("convert", float_value, int_value1, int_value2)
        self.thread.result_ready.connect(self.update_response)
        self.thread.error_occurred.connect(self.show_error)
        self.thread.connection_status.connect(self.update_connection_status)
        self.thread.finished.connect(self.stop_loading)
        self.thread.start()

    def perform_arithmetic_operation(self):
        num1 = self.get_entry_value(self.entry_num1)
        num2 = self.get_entry_value(self.entry_num2)
        num3 = self.get_entry_value(self.base)
        operation = self.get_entry_value(self.entry_operation)

        if not num1 or not num2 or not operation:
            QMessageBox.warning(self, "Внимание", "Пожалуйста, заполните все поля.")
            return

        self.start_loading()

        self.thread = NetworkThread("arif", num1, num2, num3, operation)
        self.thread.result_ready.connect(self.update_arithmetic_response)
        self.thread.error_occurred.connect(self.show_error)
        self.thread.connection_status.connect(self.update_connection_status)
        self.thread.finished.connect(self.stop_loading)
        self.thread.start()

    def update_response(self, response):
        self.text_response.setPlainText(response)

    def update_arithmetic_response(self, response):
        self.text_response_arithmetic.setPlainText(response)

    def show_error(self, message):
        QMessageBox.critical(self, "Ошибка", message)

    def start_loading(self):
        self.send_button.setEnabled(False)
        self.calculate_button.setEnabled(False)
        self.progress_bar.setVisible(True)
        self.progress_bar_arithmetic.setVisible(True)
        self.progress_bar.setRange(0, 0)  # Индикатор бесконечного прогресса
        self.progress_bar_arithmetic.setRange(0, 0)

    def stop_loading(self):
        self.send_button.setEnabled(True)
        self.calculate_button.setEnabled(True)
        self.progress_bar.setVisible(False)
        self.progress_bar_arithmetic.setVisible(False)

    def load_from_file(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Открыть файл", "", "Текстовые файлы (*.txt);;Все файлы (*.*)"
        )
        if file_path:
            try:
                with open(file_path, 'r', encoding='utf-8') as file:
                    data = file.read().replace(',', ' ').split()
                    self.set_entry_value(self.entry_float, data[0])
                    try:
                        self.set_entry_value(self.entry_int1, data[1])
                    except Exception:
                        pass
                    try:
                        self.set_entry_value(self.entry_int2, data[2])
                    except Exception:
                        pass

            except Exception as e:
                QMessageBox.critical(self, "Ошибка", f"Не удалось загрузить файл: {e}")

    def save_response(self):
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Сохранить ответ", "", "Текстовые файлы (*.txt);;Все файлы (*.*)"
        )
        if file_path:
            try:
                response_text = self.text_response.toPlainText().strip()
                with open(file_path, 'w', encoding='utf-8') as file:
                    file.write(response_text)
                QMessageBox.information(self, "Успех", "Ответ успешно сохранен.")
            except Exception as e:
                QMessageBox.critical(self, "Ошибка", f"Не удалось сохранить файл: {e}")

    def update_connection_status(self, status, color):
        self.connection_label.setText(status)
        self.connection_label.setStyleSheet(f"color: {color};")

def main():
    app = QApplication(sys.argv)
    window = ConverterWindow()
    window.show()
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()