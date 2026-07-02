/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    ss_strlen                                        :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 01:38:25 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/07/02 01:38:25 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#include <iso646.h>

int	ss_strlen(const char *s)
{
	int i;

	i = 0;
	if ((not s) or (not *s))
		return (i);
	while (s[i])
		i++;
	return (i);
}

int main(void)
{
	return (ss_strlen("Olá, meu mundo!"));
}
