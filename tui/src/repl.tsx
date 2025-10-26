import { Box, useApp, Text } from 'ink';
import React, { useState, useMemo } from 'react';
import { useRuntime } from '@/context/runtime_context.js';
import { Input } from '@/input.js';
import { Repl as ReplEngine } from '@/runtime/repl.js';

export type ReplProps = Record<string, never>;

export const Repl: React.FC<ReplProps> = () => {
	const { exit } = useApp();
	const runtime = useRuntime();
	const repl = useMemo(() => new ReplEngine(runtime), [runtime]);
	const [message, setMessage] = useState<string | null>(null);

	const handleSubmit = async (value: string) => {
		const trimmed = value.trim();

		if (!trimmed) {
			return;
		}

		if (trimmed === '/q') {
			setMessage('Bye.');
			exit();
			return;
		}

		const result = await repl.execute(trimmed);
		if (!result) {
			setMessage(`Failed to execute: ${trimmed}`);
			return;
		}

		setMessage(repl.formatResult(result));
	};

	return (
		<Box flexDirection="column" padding={1}>
			<Box>
				<Text color="gray">&gt; </Text>
				<Input onSubmit={handleSubmit} />
			</Box>
			{message && (
				<Box marginTop={1}>
					<Text color="yellow">{message}</Text>
				</Box>
			)}
		</Box>
	);
};
