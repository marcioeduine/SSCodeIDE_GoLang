#!/usr/bin/env python3
import sys
import subprocess

def generate_response(prompt):
    try:
        # Utiliza o qwen2.5-coder (ou outro modelo à tua escolha instalado no Ollama)
        cmd = ["ollama", "run", "qwen2.5-coder:1.5b", 
               f"Tu és um assistente de IA integrado num editor de texto minimalista. "
               f"Responde de forma extremamente directa, focada e apenas com código ou correções sucintas. "
               f"Prompt: {prompt}"]
        
        result = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8')
        return result.stdout.strip()
    except Exception as e:
        return f"// Erro na ligação ao modelo de IA local: {str(e)}"

if __name__ == "__main__":
    if len(sys.argv) > 1:
        user_prompt = sys.argv[1]
        print(generate_response(user_prompt))
    else:
        print("// Aviso: Nenhum prompt foi fornecido.")