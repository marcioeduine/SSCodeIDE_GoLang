#!/usr/bin/env python3
import sys
import json
import urllib.request
import os
import subprocess
from urllib.error import URLError, HTTPError

CONTEXT_FILE = "/tmp/ss_ai_context.json"
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
OLLAMA_URL = os.environ.get("SS_OLLAMA_URL", "http://localhost:11434/api/chat")
OLLAMA_MODEL = os.environ.get("SS_OLLAMA_MODEL", "qwen2.5-coder:1.5b")
LLAMA_BIN = os.environ.get("SS_LLAMA_BIN", os.path.join(BASE_DIR, "third_party/llama.cpp/build/bin/llama-cli"))
LLAMA_MODEL_FILE = os.environ.get("SS_LLAMA_MODEL_FILE", os.path.join(BASE_DIR, "models/qwen-0.5b-coder.gguf"))

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
    except OSError:
        pass

def extract_llama_cli_reply(raw_output):
    if not raw_output:
        return ""

    lines = raw_output.replace("\r", "").split("\n")
    start_idx = -1
    end_idx = len(lines)
    reply_lines = []
    line = ""

    for i, line in enumerate(lines):
        if line.startswith("> "):
            start_idx = i + 1
            break
    if start_idx == -1:
        return raw_output.strip()

    for i in range(start_idx, len(lines)):
        line = lines[i].strip()
        if line.startswith("[ Prompt:") or line == "Exiting...":
            end_idx = i
            break

    reply_lines = lines[start_idx:end_idx]
    while reply_lines and not reply_lines[0].strip():
        reply_lines.pop(0)
    while reply_lines and not reply_lines[-1].strip():
        reply_lines.pop()
    return "\n".join(reply_lines).strip()

def generate_with_ollama(history):
    payload = {
        "model": OLLAMA_MODEL,
        "messages": history,
        "stream": False,
        "options": {
            "temperature": 0.3
        }
    }

    req = urllib.request.Request(
        OLLAMA_URL,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )
    with urllib.request.urlopen(req, timeout=30) as response:
        res_data = json.loads(response.read().decode("utf-8"))
        return res_data["message"]["content"].strip()

def generate_with_llama_cpp(history):
    if not os.path.isfile(LLAMA_BIN):
        raise FileNotFoundError("llama-cli não encontrado em: " + LLAMA_BIN)
    if not os.path.isfile(LLAMA_MODEL_FILE):
        raise FileNotFoundError("modelo GGUF não encontrado em: " + LLAMA_MODEL_FILE)

    system_prompt = ""
    user_prompt = ""
    item = {}
    role = ""

    for item in history:
        role = item.get("role", "")
        if role == "system" and not system_prompt:
            system_prompt = item.get("content", "")
        if role == "user":
            user_prompt = item.get("content", "")

    cmd = [
        LLAMA_BIN,
        "-m", LLAMA_MODEL_FILE,
        "-cnv",
        "-st",
        "-sys", system_prompt,
        "-p", user_prompt,
        "-n", "256",
        "--temp", "0.3",
        "-c", "512",
        "--no-display-prompt",
        "--no-perf",
        "--simple-io",
        "--log-disable"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    output = (proc.stdout or "") + ("\n" + proc.stderr if proc.stderr else "")
    output = output.strip()
    reply = extract_llama_cli_reply(output)

    if not reply:
        if not output:
            output = "sem saída do modelo"
        raise RuntimeError("falha no llama.cpp: " + output)
    return reply

def generate_response(prompt):
    if prompt == "__CLEAR_CONTEXT__":
        if os.path.exists(CONTEXT_FILE):
            os.remove(CONTEXT_FILE)
        return ""

    history = load_context()
    history.append({"role": "user", "content": prompt})
    assistant_message = ""

    try:
        assistant_message = generate_with_ollama(history)
    except (URLError, HTTPError, TimeoutError, json.JSONDecodeError, KeyError, ValueError) as ollama_error:
        try:
            assistant_message = generate_with_llama_cpp(history)
        except (OSError, RuntimeError, FileNotFoundError) as llama_error:
            return (
                "// Erro no backend IA.\n"
                f"// Ollama: {str(ollama_error)}\n"
                f"// llama.cpp: {str(llama_error)}"
            )

    history.append({"role": "assistant", "content": assistant_message})
    save_context(history)
    return assistant_message.strip()

if __name__ == "__main__":
    if len(sys.argv) > 1:
        user_prompt = sys.argv[1]
        print(generate_response(user_prompt))
