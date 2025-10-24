import React, { useMemo } from 'react';
import { Box, Text } from 'ink';
import { Repl } from '@/Repl.js';
import { RuntimeProvider } from '@/context/RuntimeContext.js';

export type AppProps = Record<string, never>;

export const App: React.FC<AppProps> = () => {
	const runtime = useMemo(() => {
		const executors = new Map();
		return { executors };
	}, []);

	return (
		<RuntimeProvider runtime={runtime}>
			<Box flexDirection="column" height="100%">
				<Box flexGrow={1} padding={1}>
					<Text color="gray">Pane area (todo)</Text>
				</Box>
				<Box flexShrink={0}>
					<Repl />
				</Box>
			</Box>
		</RuntimeProvider>
	);
};
