"use client";

import { useEffect, useState } from 'react';
import { ref, onValue, off } from 'firebase/database';
import { db } from '@/firebase';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { ScrollArea } from '@/components/ui/scroll-area';
import { Badge } from '@/components/ui/badge';

interface Log {
  message: string;
  type: string;
  timestamp: string;
}

export function LogViewer() {
  const [logs, setLogs] = useState<Log[]>([]);

  useEffect(() => {
    const logsRef = ref(db, 'logs');
    
    const unsubscribe = onValue(logsRef, (snapshot) => {
      const data = snapshot.val();
      if (data) {
        const logsArray = Object.entries(data).map(([key, value]: [string, any]) => ({
          ...value,
          timestamp: key
        }));
        // Ordena os logs por timestamp (mais recentes primeiro)
        logsArray.sort((a, b) => parseInt(b.timestamp) - parseInt(a.timestamp));
        setLogs(logsArray);
      } else {
        setLogs([]);
      }
    });

    return () => {
      off(logsRef);
    };
  }, []);

  const getTypeColor = (type: string) => {
    switch (type) {
      case 'error':
        return 'destructive';
      case 'info':
        return 'default';
      case 'stream':
        return 'secondary';
      case 'capture':
        return 'success';
      case 'transmit':
        return 'warning';
      default:
        return 'default';
    }
  };

  return (
    <Card className="w-full">
      <CardHeader>
        <CardTitle>Logs do Sistema</CardTitle>
      </CardHeader>
      <CardContent>
        <ScrollArea className="h-[600px] w-full rounded-md border p-4">
          <div className="space-y-2">
            {logs.map((log, index) => (
              <div key={index} className="flex items-start space-x-2">
                <Badge variant={getTypeColor(log.type) as any} className="mt-1">
                  {log.type}
                </Badge>
                <div className="flex-1">
                  <p className="text-sm">{log.message}</p>
                  <p className="text-xs text-muted-foreground">
                    {new Date(parseInt(log.timestamp)).toLocaleString()}
                  </p>
                </div>
              </div>
            ))}
            {logs.length === 0 && (
              <p className="text-center text-muted-foreground">Nenhum log disponível</p>
            )}
          </div>
        </ScrollArea>
      </CardContent>
    </Card>
  );
} 