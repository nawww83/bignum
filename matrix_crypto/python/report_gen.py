import os
import sys
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_CENTER, TA_LEFT, TA_JUSTIFY
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

import matrix_crypto_core as core

# --- ЦВЕТОВАЯ ПАЛИТРА ДИЗАЙНА ("Криптографическая Ночь") ---
PRIMARY_COLOR = colors.HexColor("#1A252C")    # Глубокий сине-серый (Заголовки)
SECONDARY_COLOR = colors.HexColor("#34495E")  # Стальной синий (Подзаголовки и колонтитулы)
ACCENT_COLOR = colors.HexColor("#1ABC9C")     # Мятный крипто-зеленый (Боковые маркеры)
TEXT_COLOR = colors.HexColor("#2C3E50")       # Темно-графитовый (Основной текст)
BG_LIGHT = colors.HexColor("#F8F9FA")         # Светлая подложка под формулы
BORDER_COLOR = colors.HexColor("#BDC3C7")     # Нейтральный серый для разделительных линий

def register_cyrillic_fonts():
    """
    Автоматически находит системный TrueType шрифт с поддержкой кириллицы,
    регистрирует его и связывает в единое Font Family для предотвращения падения парсера.
    """
    possible_paths = [
        # Windows
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\arialbd.ttf",
        # macOS
        "/Library/Fonts/Arial.ttf",
        "/Library/Fonts/Arial Bold.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        # Linux (Ubuntu/Debian)
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    ]
    
    font_path = None
    bold_font_path = None
    
    # Раздельный поиск обычного и жирного начертаний
    for path in possible_paths:
        if os.path.exists(path):
            if "bd" in path.lower() or "bold" in path.lower():
                bold_font_path = path
            else:
                font_path = path

    # Если жирный не найден, дублируем обычный
    if font_path and not bold_font_path:
        bold_font_path = font_path

    if font_path:
        # 1. Регистрация базовых начертаний в движке
        pdfmetrics.registerFont(TTFont('CryptoSans', font_path))
        pdfmetrics.registerFont(TTFont('CryptoSans-Bold', bold_font_path))
        pdfmetrics.registerFont(TTFont('CryptoMono', font_path))
        
        # 2. Объединяем начертания в официальную Font Family группу для парсера HTML-тегов
        pdfmetrics.registerFontFamily('CryptoSans', normal='CryptoSans', bold='CryptoSans-Bold')
        print(f"[+] Шрифты успешно зарегистрированы и связаны в семейство 'CryptoSans'.")
    else:
        print("[-] Системные шрифты не найдены. Попытка загрузить локальный 'arial.ttf'...")
        try:
            pdfmetrics.registerFont(TTFont('CryptoSans', 'arial.ttf'))
            pdfmetrics.registerFont(TTFont('CryptoSans-Bold', 'arial.ttf'))
            pdfmetrics.registerFont(TTFont('CryptoMono', 'arial.ttf'))
            pdfmetrics.registerFontFamily('CryptoSans', normal='CryptoSans', bold='CryptoSans-Bold')
            print("[+] Локальный шрифт 'arial.ttf' успешно связан в семейство.")
        except Exception:
            print("[-] КРИТИЧЕСКАЯ ОШИБКА: Кириллический шрифт недоступен. Откат на встроенные PDF-шрифты (кириллица НЕ поддерживается).")
            # ИСПРАВЛЕНО: TTFont('CryptoSans', 'Helvetica') падал с ошибкой, так как
            # TTFont ожидает путь к реальному .ttf-файлу, а не имя шрифта.
            # Регистрируем алиасы штатных Type1-шрифтов reportlab под теми же
            # именами, которые ожидает вёрстка.
            pdfmetrics.registerFont(pdfmetrics.Font('CryptoSans', 'Helvetica', 'WinAnsiEncoding'))
            pdfmetrics.registerFont(pdfmetrics.Font('CryptoSans-Bold', 'Helvetica-Bold', 'WinAnsiEncoding'))
            pdfmetrics.registerFont(pdfmetrics.Font('CryptoMono', 'Courier', 'WinAnsiEncoding'))
            pdfmetrics.registerFontFamily('CryptoSans', normal='CryptoSans', bold='CryptoSans-Bold')

def init_crypto_styles():
    """Инициализирует стили текста с использованием связанных имен кириллических семейств"""
    styles = getSampleStyleSheet()
    
    styles.add(ParagraphStyle(
        name='DocTitle', fontName='CryptoSans-Bold', fontSize=22, leading=26,
        textColor=PRIMARY_COLOR, alignment=TA_CENTER, spaceAfter=15
    ))
    styles.add(ParagraphStyle(
        name='DocSubTitle', fontName='CryptoSans', fontSize=12, leading=16,
        textColor=SECONDARY_COLOR, alignment=TA_CENTER, spaceAfter=30
    ))
    styles.add(ParagraphStyle(
        name='SectionHeader', fontName='CryptoSans-Bold', fontSize=15, leading=19,
        textColor=PRIMARY_COLOR, spaceBefore=15, spaceAfter=10, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        name='SubSectionHeader', fontName='CryptoSans-Bold', fontSize=12, leading=16,
        textColor=SECONDARY_COLOR, spaceBefore=12, spaceAfter=6, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        name='CryptoBody', fontName='CryptoSans', fontSize=10, leading=15,
        textColor=TEXT_COLOR, alignment=TA_JUSTIFY, spaceAfter=10
    ))
    styles.add(ParagraphStyle(
        name='MathText', fontName='CryptoMono', fontSize=9.5, leading=14,
        textColor=colors.HexColor("#1A202C"), alignment=TA_LEFT
    ))
    return styles

class NumberedCanvas(canvas.Canvas):
    """Двухпроходный канвас для автоматической нумерации страниц и отрисовки колонтитулов"""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        # Кэшируем состояние страницы для финального прохода
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super().showPage()
        super().save()

    def draw_page_decorations(self, page_count):
        self.saveState()
        self.setFont("CryptoSans", 9)
        self.setFillColor(SECONDARY_COLOR)
        
        # Верхний колонтитул (Header)
        self.drawString(54, 750, "Спецификация Матричной Криптосистемы 3х3")
        self.setStrokeColor(BORDER_COLOR)
        self.setLineWidth(0.5)
        self.line(54, 742, 558, 742)
        
        # Нижний колонтитул (Footer)
        self.line(54, 45, 558, 45)
        page_text = f"Страница {self._pageNumber} из {page_count}"
        self.drawRightString(558, 30, page_text)
        self.drawString(54, 30, "Конфиденциально — Спецификация Алгоритма")
        self.restoreState()

def create_math_block(text, styles):
    """Создает изолированную таблицу-контейнер для моноширинного вывода формул и матриц"""
    p = Paragraph(text, styles['MathText'])
    t = Table([[p]], colWidths=[504])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), BG_LIGHT),
        ('PADDING', (0,0), (-1,-1), 12),
        ('ALIGN', (0,0), (-1,-1), 'LEFT'),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('LINELEFT', (0,0), (0,-1), 3, ACCENT_COLOR),
        ('BOX', (0,0), (-1,-1), 0.5, colors.HexColor("#E2E8F0")),
    ]))
    return t

def generate_pdf_report():
    """Основная сборочная функция документа"""
    pdf_filename = "matrix_crypto_specification.pdf"
    
    # 1. Сначала строго регистрируем и связываем шрифты в системе
    register_cyrillic_fonts()
    
    # 2. Настраиваем геометрию печатного листа
    doc = SimpleDocTemplate(
        pdf_filename, pagesize=letter,
        leftMargin=54, rightMargin=54, topMargin=72, bottomMargin=72
    )

    styles = init_crypto_styles()
    story = []

    # --- ДАННЫЕ О ПАРАМЕТРАХ (берутся из ядра) ---
    P_BITS = core.P_MODULO.bit_length()
    Q_BITS = core.ORDER_Q.bit_length()
    FP_P = core.sha256_hex(str(core.P_MODULO).encode("utf-8"))
    FP_T = core.sha256_hex(core.serialize_matrix(core.BASE_T))

    # --- ТИТУЛЬНЫЙ СЕГМЕНТ ---
    story.append(Spacer(1, 20))
    story.append(Paragraph("Матричная криптосистема 3х3 на подгруппе простого порядка в SL3(F_p)", styles['DocTitle']))
    story.append(Paragraph("Спецификация v2: схема Эль-Гамаля и подпись Шнорра над группой &lt;T&gt;", styles['DocSubTitle']))

    intro_text = (
        "Настоящий документ описывает <b>перепроектированную</b> версию матричной криптосистемы 3 x 3. "
        "В отличие от ранней демонстрационной схемы, здесь закрытый ключ <b>необходим</b> и для "
        "расшифрования, и для формирования подписи. В основе лежит задача дискретного логарифмирования "
        "в циклической подгруппе <b>&lt;T&gt;</b> простого порядка q группы SL<sub>3</sub>(F<sub>p</sub>). "
        "Подгруппа изоморфна норме-1 части F<sub>p^3</sub>*, поэтому параметр p выбран большим, чтобы "
        "метод index calculus не был проще общих (generic) атак."
    )
    story.append(Paragraph(intro_text, styles['CryptoBody']))
    story.append(Spacer(1, 10))

    # --- РАЗДЕЛ 1: ПАРАМЕТРЫ ---
    story.append(Paragraph("1. Системные параметры и ключи", styles['SectionHeader']))
    story.append(Paragraph(
        "Общеизвестные параметры генерируются офлайн (matrix_gen.py) и хранятся в matrix_params.py. "
        "Для гарантии делимости порядка поля используется кубический корень из единицы:", styles['CryptoBody']
    ))

    params_text = (
        f"p  - простое, {P_BITS} бит          (matrix_params.P_MODULO)<br/>"
        f"q  - простое, {Q_BITS} бит, q = 1 (mod 3)    (matrix_params.GROUP_ORDER_Q)<br/>"
        f"    q = {core.ORDER_Q}<br/>"
        "w  - кубический корень из 1 по модулю q:   w^2 + w + 1 = 0 (mod q)<br/>"
        "p = w (mod q)   =>   q | p^2 + p + 1<br/><br/>"
        "T  - генератор &lt;T&gt; в SL3(F_p), order(T) = q   (matrix_params.BASE_T)<br/>"
        f"SHA-256(p) = {FP_P}<br/>"
        f"SHA-256(T) = {FP_T}"
    )
    story.append(create_math_block(params_text, styles))

    story.append(Paragraph("Закрытый ключ (Private Key):", styles['SubSectionHeader']))
    story.append(create_math_block("Private Key = x,    x - случайное из [1, q-1]", styles))

    story.append(Paragraph("Открытый ключ (Public Key):", styles['SubSectionHeader']))
    story.append(create_math_block("Public Key = Y = T^x   (mod p)", styles))

    story.append(PageBreak())

    # --- РАЗДЕЛ 2: ШИФРОВАНИЕ ---
    story.append(Paragraph("2. Шифрование (Эль-Гамаль в &lt;T&gt;)", styles['SectionHeader']))
    story.append(Paragraph("Отправитель знает только открытый ключ Y:", styles['CryptoBody']))
    enc_text = (
        "Блок сообщения:      P  - матрица 3x3 из элементов F_p<br/>"
        "Одноразовый секрет:  r - случайное из [1, q-1]<br/><br/>"
        "C1 = T^r<br/>"
        "S  = Y^r = T^(x*r)<br/>"
        "C2 = P * S<br/><br/>"
        "Шифртекст = (C1, C2)"
    )
    story.append(create_math_block(enc_text, styles))

    story.append(PageBreak())

    # --- РАЗДЕЛ 3: ДЕШИФРОВАНИЕ ---
    story.append(Paragraph("3. Дешифрование (требуется секрет x)", styles['SectionHeader']))
    dec_text = (
        "Боб знает x, поэтому восстанавливает S из первого компонента:<br/><br/>"
        "S  = C1^x = T^(r*x) = Y^r<br/>"
        "P  = C2 * S^(-1)<br/><br/>"
        "Вычислить S из C1 = T^r без знания x равносильно решению DLP в &lt;T&gt;."
    )
    story.append(create_math_block(dec_text, styles))

    story.append(PageBreak())

    # --- РАЗДЕЛ 4: ПОДПИСЬ ---
    story.append(Paragraph("4. Электронная цифровая подпись (схема Шнорра в &lt;T&gt;)", styles['SectionHeader']))
    story.append(Paragraph("Формирование подписи (владелец секрета x):", styles['SubSectionHeader']))
    sign_text = (
        "k - случайное из [1, q-1]          (одноразовый нонс)<br/>"
        "R   = T^k<br/>"
        "e   = SHA-512( Serialize(R) || Message )  mod q<br/>"
        "s   = k + e*x   mod q<br/><br/>"
        "Подпись = (R, s)"
    )
    story.append(create_math_block(sign_text, styles))

    story.append(Paragraph("Проверка подписи (любой, кто знает Y):", styles['SubSectionHeader']))
    verify_text = (
        "e = SHA-512( Serialize(R) || Message )  mod q<br/>"
        "Проверка:   T^s  ==  R * Y^e    (mod p)<br/><br/>"
        "Подделка без знания x вычислительно эквивалентна DLP в &lt;T&gt;."
    )
    story.append(create_math_block(verify_text, styles))

    # --- РАЗДЕЛ 5: СТОЙКОСТЬ ---
    story.append(Paragraph("5. Оценка стойкости (честная)", styles['SectionHeader']))
    sec_text = (
        "1. q простое => нет атак Поллига-Хеллмана и атак на малые подгруппы.<br/>"
        f"2. Общие (generic) атаки на DLP: ~ sqrt(q) ~ 2^{Q_BITS // 2} операций.<br/>"
        f"3. &lt;T&gt; изоморфна подгруппе F_(p^3)*, поэтому p выбран большим:<br/>"
        f"   p^3 ~ 2^{P_BITS * 3} бит (защита от index calculus / NFS-DL).<br/>"
        "4. Схема НЕ устойчива к квантовому компьютеру (как ECDH / ECDSA)."
    )
    story.append(create_math_block(sec_text, styles))

    # --- РАЗДЕЛ 6: АУТЕНТИФИКАЦИЯ ШИФРТЕКСТА ---
    story.append(Paragraph("6. Аутентификация шифртекста (encrypt-then-sign)", styles['SectionHeader']))
    story.append(Paragraph(
        "Эль-Гамаль обеспечивает только конфиденциальность и мультипликативно изменяем. Чтобы "
        "получатель мог отличить подлинный шифртекст от изменённого, применяется композиция "
        "«сначала зашифровать, затем подписать»:", styles['CryptoBody']
    ))
    auth_text = (
        "Атака на сырую схему (без ключа):<br/>"
        "    C2' = Z * C2   =>   decrypt:   C2' * (C1^x)^(-1) = Z * P<br/><br/>"
        "Конверт encrypt-then-sign:<br/>"
        "    ct     = (C1, C2) = encrypt(Y_получателя, P)<br/>"
        "    sig    = Schnorr_sign(x_отправителя, ct)<br/>"
        "    sealed = base64( { v, ct, sig } )<br/><br/>"
        "Получатель:  Verify(Y_отправителя, ct, sig) ?  decrypt(x_получателя, ct)  :  reject<br/>"
        "Любая правка ct ломает подпись => шифртекст отвергается."
    )
    story.append(create_math_block(auth_text, styles))

    # Компиляция документа
    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"[+] Отчет успешно сгенерирован: {pdf_filename}")

# --- ТОЧКА ВХОДА ---
if __name__ == "__main__":
    generate_pdf_report()
