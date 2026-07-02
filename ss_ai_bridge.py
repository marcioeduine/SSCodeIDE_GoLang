#!/usr/bin/env python3
import sys
import json
import urllib.request
import os

CONTEXT_FILE = "/tmp/ss_ai_context.json"

def load_context():
    if os.path.exists(CONTEXT_FILE):
        try:
            with open(CONTEXT_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except:
            pass
    # Prompt de sistema optimizado e imperativo para o modelo local
    return [{
        "role": "system", 
        "content": (
            "Tu és um assistente de IA integrado num painel lateral de um editor de texto. "
            "OBRIGATÓRIO: Responde APENAS em Português de Portugal (pt-PT), usando estritamente a norma culta, "
            "tratando o utilizador por 'tu' e aplicando rigidamente a grafia do PRÉ-ACORDO ORTOGRÁFICO DE 2012. "
            "Deves manter todas as consoantes mudas (exemplos obrigatórios: acção, directo, correcto, óptimo, "
            "inspecção, interfaço, baptismo, Julho). Nunca uses termos ou construções do Brasil (usa comprimento "
            "em vez de tamanho, e caractere ou caracteres correctamente). "
            "Os blocos de código e explicações técnicas devem ser limpos, directos e em inglês técnico (en-GB) se aplicável."
        )
    }]

def save_context(history):
    try:
        with open(CONTEXT_FILE, "w", encoding="utf-8") as f:
            json.dump(history, f, ensure_ascii=False, indent=2)
    except:
        pass

def generate_response(prompt):
    if prompt == "__CLEAR_CONTEXT__":
        if os.path.exists(CONTEXT_FILE):
            os.remove(CONTEXT_FILE)
        return ""

    history = load_context()
    history.append({"role": "user", "content": prompt})

    url = "http://localhost:11434/api/chat"
    payload = {
        "model": "qwen2.5-coder:1.5b",
        "messages": history,
        "stream": False,
        "options": {
            "temperature": 0.3 # Temperatura mais baixa reduz desvios de comportamento do modelo
        }
    }
    
    try:
        req = urllib.request.Request(
            url, 
            data=json.dumps(payload).encode("utf-8"), 
            headers={"Content-Type": "application/json"}
        )
        with urllib.request.urlopen(req) as response:
            res_data = json.loads(response.read().decode("utf-8"))
            assistant_message = res_data["message"]["content"]
            
            history.append({"role": "assistant", "content": assistant_message})
            save_context(history)
            
            return assistant_message.strip()
    except Exception as e:
        return f"// Erro na ligação à API do Ollama: {str(e)}"

if __name__ == "__main__":
    if len(sys.argv) > 1:
        user_prompt = sys.argv[1]
        print(generate_response(user_prompt))
