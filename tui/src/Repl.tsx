import { Box, useApp, Text } from 'ink';
import { TextInput } from '@inkjs/ui';
import React, { useState } from 'react';

export type ReplProps = Record<string, never>;

export const Repl: React.FC<ReplProps> = () => {
	const { exit } = useApp();
	const [message, setMessage] = useState<string | null>(null);
	const [resetCounter, setResetCounter] = useState(0);

	const handleSubmit = (value: string) => {
		const trimmed = value.trim();

		if (!trimmed) {
			return;
		}

		if (trimmed === '/q') {
			setMessage('Bye.');
			exit();
			return;
		}

		setMessage(`Unknown command: ${trimmed}`);
		setResetCounter((c) => c + 1);
	};

	return (
		<Box flexDirection="column" padding={1}>
			<Box>
				<Text color="gray">&gt; </Text>
				<TextInput key={resetCounter} onSubmit={handleSubmit} />
			</Box>
			{message && (
				<Box marginTop={1}>
					<Text color="yellow">{message}</Text>
				</Box>
			)}
		</Box>
	);
};
