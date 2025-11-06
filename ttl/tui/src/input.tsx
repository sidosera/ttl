import React, { useState } from 'react';
import { Text, useInput } from 'ink';
import { useHistory } from '@/hooks/use_history.js';

type HistoryInputProps = {
	onSubmit: (value: string) => void;
};

export const Input: React.FC<HistoryInputProps> = ({ onSubmit }) => {
	const [input, setInput] = useState('');
	const [cursorPosition, setCursorPosition] = useState(0);
	const { addToHistory, navigateHistory } = useHistory();

	useInput(async (char, key) => {
		if (key.return) {
			if (input.trim()) {
				await addToHistory(input);
				onSubmit(input);
				setInput('');
				setCursorPosition(0);
			}
		} else if (key.backspace || key.delete) {
			if (cursorPosition > 0) {
				setInput(input.slice(0, cursorPosition - 1) + input.slice(cursorPosition));
				setCursorPosition(cursorPosition - 1);
			}
		} else if (key.leftArrow) {
			setCursorPosition(Math.max(0, cursorPosition - 1));
		} else if (key.rightArrow) {
			setCursorPosition(Math.min(input.length, cursorPosition + 1));
		} else if (key.upArrow) {
			const command = await navigateHistory('up');
			if (command !== null) {
				setInput(command);
				setCursorPosition(command.length);
			}
		} else if (key.downArrow) {
			const command = await navigateHistory('down');
			if (command !== null) {
				setInput(command);
				setCursorPosition(command.length);
			}
		} else if (!key.ctrl && !key.meta && char) {
			const newInput = input.slice(0, cursorPosition) + char + input.slice(cursorPosition);
			setInput(newInput);
			setCursorPosition(cursorPosition + 1);
		}
	});

	if (input.length === 0) {
		return <Text inverse> </Text>;
	}

	const beforeCursor = input.slice(0, cursorPosition);
	const atCursor = input[cursorPosition] || ' ';
	const afterCursor = input.slice(cursorPosition + 1);

	return (
		<Text>
			{beforeCursor}<Text inverse>{atCursor}</Text>{afterCursor}
		</Text>
	);
};
