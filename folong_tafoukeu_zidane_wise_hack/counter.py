import re

def count_emojis(file_path):
    # Regex couvrant la majorité des emojis Unicode
    emoji_pattern = re.compile(
        "["
        "\U0001F600-\U0001F64F"  # Emoticônes
        "\U0001F300-\U0001F5FF"  # Symboles & pictogrammes
        "\U0001F680-\U0001F6FF"  # Transport & cartes
        "\U0001F700-\U0001F77F"
        "\U0001F780-\U0001F7FF"
        "\U0001F800-\U0001F8FF"
        "\U0001F900-\U0001F9FF"
        "\U0001FA00-\U0001FAFF"
        "\U00002700-\U000027BF"  # Dingbats
        "\U00002600-\U000026FF"  # Symboles divers
        "]",
        flags=re.UNICODE
    )

    with open(file_path, "r", encoding="utf-8") as f:
        text = f.read()

    emojis = emoji_pattern.findall(text)
    return len(emojis)

# Exemple d'utilisation
if __name__ == "__main__":
    file_path = "texte.txt"
    print(f"Nombre d'émojis : {count_emojis(file_path)}")
