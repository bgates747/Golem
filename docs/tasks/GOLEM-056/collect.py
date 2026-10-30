"""Refresh consultation delivery/return records; never send or acknowledge mail."""
from pathlib import Path
import hashlib
import json

HERE = Path(__file__).resolve().parent
MAILBOX = HERE.parents[3] / 'agon-dev-env/cross-agent/agentcoms'
THREAD = 'golem056-capability-review'


def messages():
    result = []
    for path in sorted((MAILBOX / 'messages').glob('*.md')):
        _, header, body = path.read_text().split('---\n', 2)
        row = dict((key, json.loads(value)) for key, value in
                   (line.split(': ', 1) for line in header.splitlines()))
        if row['thread_id'] == THREAD:
            row['file'] = str(path)
            row['sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()
            row['body'] = body.removeprefix('\n')
            result.append(row)
    return result


def main():
    roster = json.loads((HERE / 'roster.json').read_text())
    notes_path = HERE / 'assessments.json'
    notes = json.loads(notes_path.read_text()) if notes_path.exists() else {}
    rows = messages()
    invitations = {item['invitation']: item['recipient'] for item in roster}
    owners = dict(invitations)
    unresolved = []
    for row in rows:
        parent = row.get('in_reply_to')
        if parent in owners:
            owners[row['message_id']] = owners[parent]
    for row in rows:
        if row['recipient'] == 'golem' and row['message_id'] not in owners:
            exact = [r['recipient'] for r in roster if r['recipient'] == row['author']]
            if len(exact) == 1:
                owners[row['message_id']] = exact[0]
            else:
                unresolved.append(row['message_id'])
    returns = []
    for item in roster:
        received = [row for row in rows if row['recipient'] == 'golem' and
                    row['author'] != 'golem' and
                    owners.get(row['message_id']) == item['recipient']]
        item['returns'] = [row['message_id'] for row in received]
        assessment = notes.get(item['recipient'], {})
        reviewed = set(assessment.get('reviewed_messages', []))
        if assessment.get('reviewed_message'):
            reviewed.add(assessment['reviewed_message'])
        item['reply'] = ('reviewed' if received and all(row['message_id'] in reviewed for row in received)
                         else 'received; content review required' if received else 'pending')
        ack = MAILBOX / ('ack-' + item['recipient'] + '.json')
        read = ack.exists() and item['invitation'] in json.loads(ack.read_text())
        item['acknowledgement'] = ('reply received' if received else
                                   'mail read acknowledged' if read else 'not established')
        for row in received:
            returns.append({**row, 'session_recipient': item['recipient']})
    (HERE / 'roster.json').write_text(json.dumps(roster, indent=2) + '\n')
    (HERE / 'returns.json').write_text(json.dumps({'thread': THREAD, 'returns': returns,
                                                'unresolved': unresolved}, indent=2) + '\n')
    count = sum(bool(item['returns']) for item in roster)
    lines = ['# GOLEM-056 outreach, returns and later assignments', '',
             f'All {len(roster)} sessions received mailbox invitations and brief-only chat '
             f'notifications. {count} sessions have mailed {len(returns)} reports; '
             f'{len(roster) - count} remain outstanding. Receipt and content review '
             'are recorded separately; acknowledgement alone is not a substantive return.', '',
             '| Existing session | Mail identity | Mail read/replied | Content review | Return |',
             '| --- | --- | --- | --- | --- |']
    for item in roster:
        links = ', '.join('[' + mid.split('_', 3)[-1] + '](' + str(MAILBOX / 'messages' / (mid + '.md')) + ')'
                          for mid in item['returns']) or 'Pending'
        lines.append('| ' + item['title'] + ' | ' + item['recipient'] + ' | ' +
                     item['acknowledgement'] + ' | ' + item['reply'] + ' | ' + links + ' |')
    lines += ['', 'Invitation IDs and chat delivery receipts are in [roster.json](roster.json) '
              'and [chat-delivery.json](chat-delivery.json). Immutable return bodies, paths '
              'and hashes are indexed in [returns.json](returns.json); manual assessments '
              'are in [assessments.json](assessments.json). Consolidated proposals are in '
              '[review.md](review.md).', '',
              '## Review and assignment gate', '',
              'The Author and Golem review the combined responses after all seven returns, '
              'or an explicit Author disposition of an outstanding item. Silence never '
              'closes a row. No implementation assignment has been dispatched.', '',
              '## Assignment register', '',
              'No assignments yet. Approved entries will record a stable assignment ID, '
              'original proposal/message IDs, owner, task/contract pin, dispatch message, '
              'acknowledgement, current state, completion evidence and remaining blockers. '
              'Project TODOs remain authoritative for their own implementation work.']
    if unresolved:
        lines += ['', 'Unresolved incoming identities: ' + ', '.join(unresolved)]
    supplemental_path = HERE / 'supplemental-returns.json'
    if supplemental_path.exists():
        supplements = json.loads(supplemental_path.read_text())
        lines += ['', '## Additional handoffs outside the frozen invitation set', '',
                  'These do not change the frozen mail window, seven-session roster '
                  'or original reply approval. Details: '
                  '[supplemental-returns.json](supplemental-returns.json).', '']
        for item in supplements:
            lines.append('- ' + item['author'] + ': [' + item['message_id'] + '](' +
                         item['file'] + '). ' + item['status'] +
                         ' [Technical review](' + item['review'] + ').')
    (HERE / 'coordination.md').write_text('\n'.join(lines) + '\n')
    print(json.dumps({'sessions': len(roster), 'returned': count,
                      'outstanding': [r['recipient'] for r in roster if not r['returns']],
                      'unresolved': unresolved}))


if __name__ == '__main__':
    main()
